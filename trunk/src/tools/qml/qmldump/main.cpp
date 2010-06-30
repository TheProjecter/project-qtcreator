#include <QApplication>
#include <QSet>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include <QMetaObject>
#include <QMetaProperty>
#include <QPushButton>
#include <QDebug>
#include <iostream>
#include <QtDeclarative>
#include <QtDeclarative/private/qdeclarativemetatype_p.h>
#include <QtDeclarative/QDeclarativeView>

static QHash<QByteArray, const QDeclarativeType *> qmlTypeByCppName;
static QHash<QByteArray, QByteArray> cppToQml;

QByteArray convertToQmlType(const QByteArray &cppName)
{
    QByteArray qmlName = cppToQml.value(cppName, cppName);
    qmlName.replace("::", ".");
    qmlName.replace("/", ".");
    return qmlName;
}

void erasure(QByteArray *typeName, bool *isList, bool *isPointer)
{
    static QByteArray declListPrefix = "QDeclarativeListProperty<";

    if (typeName->endsWith('*')) {
        *isPointer = true;
        typeName->truncate(typeName->length() - 1);
        erasure(typeName, isList, isPointer);
    } else if (typeName->startsWith(declListPrefix)) {
        *isList = true;
        typeName->truncate(typeName->length() - 1); // get rid of the suffix '>'
        *typeName = typeName->mid(declListPrefix.size());
        erasure(typeName, isList, isPointer);
    }

    *typeName = convertToQmlType(*typeName);
}

void processMetaObject(const QMetaObject *meta, QSet<const QMetaObject *> *metas)
{
    if (! meta || metas->contains(meta))
        return;

    metas->insert(meta);
    processMetaObject(meta->superClass(), metas);
}

void processObject(QObject *object, QSet<const QMetaObject *> *metas)
{
    if (! object)
        return;

    const QMetaObject *meta = object->metaObject();
    processMetaObject(meta, metas);

    for (int index = 0; index < meta->propertyCount(); ++index) {
        QMetaProperty prop = meta->property(index);
        if (QDeclarativeMetaType::isQObject(prop.userType())) {
            QObject *oo = QDeclarativeMetaType::toQObject(prop.read(object));
            if (oo && !metas->contains(oo->metaObject()))
                processObject(oo, metas);
        }
    }
}

void processDeclarativeType(const QDeclarativeType *ty, QSet<const QMetaObject *> *metas)
{
    processMetaObject(ty->metaObject(), metas);
}

void writeType(QXmlStreamAttributes *attrs, QByteArray typeName)
{
    bool isList = false, isPointer = false;
    erasure(&typeName, &isList, &isPointer);
    attrs->append(QXmlStreamAttribute("type", typeName));
    if (isList)
        attrs->append(QXmlStreamAttribute("isList", "true"));
}

void dump(const QMetaProperty &prop, QXmlStreamWriter *xml)
{
    xml->writeStartElement("property");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(prop.name())));

    writeType(&attributes, prop.typeName());

    xml->writeAttributes(attributes);
    xml->writeEndElement();
}

void dump(const QMetaMethod &meth, QXmlStreamWriter *xml)
{
    if (meth.methodType() == QMetaMethod::Signal) {
        if (meth.access() != QMetaMethod::Protected)
            return; // nothing to do.
    } else if (meth.access() != QMetaMethod::Public) {
        return; // nothing to do.
    }

    QByteArray name = meth.signature();
    int lparenIndex = name.indexOf('(');
    if (lparenIndex == -1) {
        return; // invalid signature
    }

    name = name.left(lparenIndex);


    QString elementName = QLatin1String("method");

    QXmlStreamAttributes attributes;
    if (meth.methodType() == QMetaMethod::Signal)
        elementName = QLatin1String("signal");

    xml->writeStartElement(elementName);

    attributes.append(QXmlStreamAttribute("name", name));

    const QString typeName = convertToQmlType(meth.typeName());
    if (! typeName.isEmpty())
        attributes.append(QXmlStreamAttribute("type", typeName));

    xml->writeAttributes(attributes);

    for (int i = 0; i < meth.parameterTypes().size(); ++i) {
        QByteArray argName = meth.parameterNames().at(i);

        xml->writeStartElement("param");

        QXmlStreamAttributes attrs;

        if (! argName.isEmpty())
            attrs.append(QXmlStreamAttribute("name", argName));

        writeType(&attrs, meth.parameterTypes().at(i));
        xml->writeAttributes(attrs);

        xml->writeEndElement();
    }

    xml->writeEndElement();
}

void dump(const QMetaEnum &e, QXmlStreamWriter *xml)
{
    xml->writeStartElement("enum");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(e.name()))); // ### FIXME
    xml->writeAttributes(attributes);

    for (int index = 0; index < e.keyCount(); ++index) {
        xml->writeStartElement("enumerator");

        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(e.key(index))));
        attributes.append(QXmlStreamAttribute("value", QString::number(e.value(index))));
        xml->writeAttributes(attributes);

        xml->writeEndElement();
    }

    xml->writeEndElement();
}

class FriendlyQObject: public QObject
{
public:
    static const QMetaObject *qtMeta() { return &staticQtMetaObject; }
};

void dump(const QMetaObject *meta, QXmlStreamWriter *xml)
{
    xml->writeStartElement("type");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", convertToQmlType(meta->className())));

    if (const QDeclarativeType *qmlTy = qmlTypeByCppName.value(meta->className())) {
        attributes.append(QXmlStreamAttribute("version", QString("%1.%2").arg(qmlTy->majorVersion()).arg(qmlTy->minorVersion())));
    }

    for (int index = meta->classInfoCount() - 1 ; index >= 0 ; --index) {
        QMetaClassInfo classInfo = meta->classInfo(index);
        if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
            attributes.append(QXmlStreamAttribute("defaultProperty", QLatin1String(classInfo.value())));
            break;
        }
    }

    QString version;

    if (meta->superClass())
        attributes.append(QXmlStreamAttribute("extends", convertToQmlType(meta->superClass()->className())));

    if (! version.isEmpty())
        attributes.append(QXmlStreamAttribute("version", version));

    xml->writeAttributes(attributes);

    for (int index = meta->enumeratorOffset(); index < meta->enumeratorCount(); ++index)
        dump(meta->enumerator(index), xml);

    for (int index = meta->propertyOffset(); index < meta->propertyCount(); ++index)
        dump(meta->property(index), xml);

    for (int index = meta->methodOffset(); index < meta->methodCount(); ++index)
        dump(meta->method(index), xml);

    xml->writeEndElement();
}

void writeScriptElement(QXmlStreamWriter *xml)
{
    xml->writeStartElement("type");
    {
        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", "Script"));
        xml->writeAttributes(attributes);
    }

    xml->writeStartElement("property");
    {
        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", "script"));
        attributes.append(QXmlStreamAttribute("type", "string"));
        xml->writeAttributes(attributes);
    }
    xml->writeEndElement();

    xml->writeStartElement("property");
    {
        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", "source"));
        attributes.append(QXmlStreamAttribute("type", "QUrl"));
        xml->writeAttributes(attributes);
    }
    xml->writeEndElement();

    xml->writeEndElement();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QDeclarativeView view;
    QDeclarativeEngine *engine = view.engine();

    {
        QByteArray code;
        code += "import Qt 4.7;\n";
        code += "import Qt.labs.particles 4.7;\n";
        code += "import Qt.labs.gestures 4.7;\n";
        code += "import Qt.labs.folderlistmodel 4.7;\n";
        code += "import org.webkit 1.0;\n";
        code += "Item {}";
        QDeclarativeComponent c(engine);
        c.setData(code, QUrl("xxx"));
        c.create();
    }

    cppToQml.insert("QString", "string");
    cppToQml.insert("QEasingCurve", "Qt.Easing");
    cppToQml.insert("QDeclarativeEasingValueType::Type", "Type");

    QSet<const QMetaObject *> metas;

    metas.insert(FriendlyQObject::qtMeta());

    // ### TODO: We don't treat extended types correctly. Currently only hits the
    // QDeclarativeGraphicsWidget extension to QGraphicsWidget
    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        if (ty->isExtendedType())
            continue;

        cppToQml.insert(ty->metaObject()->className(), ty->qmlTypeName());
        qmlTypeByCppName.insert(ty->metaObject()->className(), ty);
        processDeclarativeType(ty, &metas);
    }

    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        if (ty->isExtendedType())
            continue;

        QByteArray tyName = ty->qmlTypeName();
        tyName = tyName.mid(tyName.lastIndexOf('/') + 1);

        QByteArray code;
        code += "import Qt 4.7;\n";
        code += "import Qt.labs.particles 4.7;\n";
        code += "import Qt.labs.gestures 4.7;\n";
        code += "import Qt.labs.folderlistmodel 4.7;\n";
        code += "import org.webkit 1.0;\n";
        code += tyName;
        code += " {}\n";

        QDeclarativeComponent c(engine);
        c.setData(code, QUrl("xxx"));
        processObject(c.create(), &metas);
    }

    QByteArray bytes;
    QXmlStreamWriter xml(&bytes);
    xml.setAutoFormatting(true);

    xml.writeStartDocument("1.0");
    xml.writeStartElement("module");

    QMap<QString, const QMetaObject *> nameToMeta;
    foreach (const QMetaObject *meta, metas) {
        nameToMeta.insert(convertToQmlType(meta->className()), meta);
    }
    foreach (const QMetaObject *meta, nameToMeta) {
        dump(meta, &xml);
    }

    writeScriptElement(&xml);

    xml.writeEndElement();
    xml.writeEndDocument();

    std::cout << bytes.constData();

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(0);
    QObject::connect(&timer, SIGNAL(timeout()), &app, SLOT(quit()));

    timer.start();

    return app.exec();
}

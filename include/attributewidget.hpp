#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "drawableattribute.hpp"

#include <QPushButton>
#include <QTreeWidget>
#include <annotation.hpp>

namespace Ui {
class AttributeWidget;
}

class AttributeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit AttributeWidget(QWidget* parent);
    ~AttributeWidget();

    void update();

    std::shared_ptr<SemantisedTriangleMesh::Annotation> getAnnotation() const;
    void setAnnotation(std::shared_ptr<SemantisedTriangleMesh::Annotation> value);

signals:
    void updateSignal();
    void updateViewSignal();
private slots:
    void measureButtonClickedSlot();
    void deletionButtonClickedSlot();

private:
    std::map<QPushButton*, std::shared_ptr<SemantisedTriangleMesh::Attribute> >  buttonMeasureMap;
    Ui::AttributeWidget *ui;
    std::shared_ptr<SemantisedTriangleMesh::Annotation> annotation;
    QTreeWidgetItem* m_pItem;
};

#endif // ATTRIBUTEWIDGET_H

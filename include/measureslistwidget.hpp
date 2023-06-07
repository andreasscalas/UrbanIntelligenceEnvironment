#ifndef MEASURESLISTWIDGET_H
#define MEASURESLISTWIDGET_H

#include <drawabletrianglemesh.hpp>

#include <QTreeWidget>

namespace Ui {
class MeasuresListWidget;
}

class MeasuresListWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit MeasuresListWidget(QWidget *parent = nullptr);
    ~MeasuresListWidget();

    void update();
    std::shared_ptr<DrawableTriangleMesh> getMesh() const;
    void setMesh(std::shared_ptr<DrawableTriangleMesh> value);

signals:
    void updateSignal();
    void updateViewSignal();

private slots:
    void updateSlot();
    void updateViewSlot();
private:
    Ui::MeasuresListWidget *ui;
    std::shared_ptr<DrawableTriangleMesh>  mesh;
};

#endif // MEASURESLISTWIDGET_H

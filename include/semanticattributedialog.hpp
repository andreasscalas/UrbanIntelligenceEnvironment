#ifndef SEMANTICATTRIBUTEDIALOG_H
#define SEMANTICATTRIBUTEDIALOG_H

#include <QDialog>

namespace Ui {
class SemanticAttributeDialog;
}

class SemanticAttributeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SemanticAttributeDialog(QWidget *parent = nullptr);
    ~SemanticAttributeDialog();

    bool getSuccess() const;
    QString getAttributeName();
    QString getAttributeValue();

signals:
    void textFinalized(std::string, std::string);
private slots:
    void on_buttonBox_accepted();

private:
    Ui::SemanticAttributeDialog *ui;
    bool success;
};

#endif // SEMANTICATTRIBUTEDIALOG_H

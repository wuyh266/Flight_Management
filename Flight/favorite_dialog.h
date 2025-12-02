#ifndef FAVORITE_DIALOG_H
#define FAVORITE_DIALOG_H

#include <QWidget>

namespace Ui {
class favorite_dialog;
}

class favorite_dialog : public QWidget
{
    Q_OBJECT

public:
    explicit favorite_dialog(QWidget *parent = nullptr);
<<<<<<< HEAD
    // 修改：接收 userID 而不是 username
    explicit favorite_dialog(const QString &userID, QWidget *parent = nullptr);
=======
    explicit favorite_dialog(const QString &username, QWidget *parent = nullptr);
>>>>>>> 8baf20252257e6424a56f672cef2e41d0128f62f

    ~favorite_dialog();
    void refreshFavoriteList();

signals:
    void backRequested();

private slots:
    void on_btn_back_clicked();
    void on_btn_refresh_clicked();
    void on_searchBtn_clicked();
    void onRemoveFavorite();

private:
    Ui::favorite_dialog *ui;
<<<<<<< HEAD
    QString currentUserID; // 修改：存储 UserID
=======
    QString currentUsername;
>>>>>>> 8baf20252257e6424a56f672cef2e41d0128f62f
    void initTable();
    void loadFavorites();
};

#endif // FAVORITE_DIALOG_H

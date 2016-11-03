#ifndef QROGUE_H
#define QROGUE_H

#include <memory>
#include <mutex>
#include <QtQuick/QQuickPaintedItem>
#include <coord.h>
#include "game_config.h"

struct Environment;
class QtRogueInput;
class QRogueDisplay;

class QRogue : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(QSize fontSize READ fontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QSize screenSize READ screenSize NOTIFY screenSizeChanged)
    Q_PROPERTY(QString game READ game WRITE setGame NOTIFY gameChanged)

public:
    QRogue(QQuickItem *parent = 0);
    ~QRogue();

    QFont font() const;
    void setFont(const QFont& font);

    QSize fontSize() const;
    QSize screenSize() const;

    QString game() const;
    void setGame(const QString& game);

    Q_INVOKABLE void restoreGame(const QString& filename);
    Q_INVOKABLE void saveGame(const QString& filename);

    virtual void paint(QPainter *painter) override;
    void postRender();

public slots:
    void onTimer();

signals:
    void render();
    void fontSizeChanged(int height, int width);
    void screenSizeChanged(int height, int width);
    void gameChanged(const QString& name);

public:
    Environment* GameEnv() const;
    Environment* CurrentEnv() const;
    QtRogueInput* Input() const;
    QRogueDisplay* Display() const;
    GameConfig Config() const;

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;

private:
    void LaunchGame();
    void SaveGame(std::string path, bool notify);
    void RestoreGame(const std::string& path);
    bool GetSavePath(std::string& filename);

    static const unsigned char kSaveVersion;

    GameConfig config_;
    std::shared_ptr<Environment> env_;
    std::shared_ptr<Environment> game_env_;
    std::unique_ptr<QtRogueInput> input_;
    std::unique_ptr<QRogueDisplay> display_;
    uint16_t restore_count_ = 0;
};

#endif

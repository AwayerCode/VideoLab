#pragma once

#include <QWidget>
#include <QPainter>
#include <vector>
#include <string>

class MP4BoxView : public QWidget {
    Q_OBJECT

public:
    struct BoxInfo {
        std::string type;    // box类型
        int64_t size;       // box大小
        int64_t offset;     // 文件偏移
        int level;          // 嵌套层级
    };

    explicit MP4BoxView(QWidget* parent = nullptr);
    ~MP4BoxView() override;

    void setBoxes(const std::vector<BoxInfo>& boxes);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QSize sizeHint() const override;

private:
    void updateLayout();
    QColor getBoxColor(const std::string& type) const;
    QString getBoxLabel(const BoxInfo& box) const;

    std::vector<BoxInfo> boxes_;
    std::vector<QRectF> boxRects_;
    int columns_{4};  // 每行显示的box数量
    int boxSize_{100};  // box的基础大小
    int spacing_{10};  // box之间的间距
}; 
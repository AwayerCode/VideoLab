#pragma once

#include <QWidget>
#include <vector>
#include "../format/h264_parser.hpp"

class H264MatrixView : public QWidget {
    Q_OBJECT

public:
    explicit H264MatrixView(QWidget* parent = nullptr);
    ~H264MatrixView() override = default;

    void setNALUnits(const std::vector<H264Parser::NALUnitInfo>& nalUnits);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QSize sizeHint() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Cell {
        H264Parser::NALUnitType type;
        bool isKeyframe;
        int64_t size;
        double timestamp;
        QRect rect;
    };

    void updateLayout();
    QColor getColorForNALType(H264Parser::NALUnitType type, bool isKeyframe) const;
    QString getTooltipText(const Cell& cell) const;
    void showTooltip(const QPoint& pos, const QString& text);

    std::vector<Cell> cells_;
    int columns_{16};  // 每行显示的单元格数
    int cellSize_{30}; // 单元格大小
    int spacing_{2};   // 单元格间距
    QPoint lastMousePos_;
}; 
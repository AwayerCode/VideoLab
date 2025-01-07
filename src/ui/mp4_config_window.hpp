#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QTextEdit>
#include <QScrollArea>
#include "mp4_box_view.hpp"

class MP4ConfigWindow : public QWidget {
    Q_OBJECT

public:
    explicit MP4ConfigWindow(QWidget* parent = nullptr);
    ~MP4ConfigWindow() override;

private slots:
    void onSelectFile();
    void onAnalyzeMP4();

private:
    void setupUI();
    void setupConnections();

    QVBoxLayout* mainLayout_;
    QHBoxLayout* fileSelectLayout_;
    QLineEdit* filePathEdit_;
    QPushButton* selectFileBtn_;
    QPushButton* analyzeBtn_;
    QTextEdit* resultDisplay_;
    QScrollArea* boxScrollArea_;
    MP4BoxView* boxView_;
}; 
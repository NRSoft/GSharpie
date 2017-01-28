#ifndef GSHARPIE_GCODEEDITOR_H
#define GSHARPIE_GCODEEDITOR_H

#include <QObject>
#include <QPlainTextEdit>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;


class GCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    GCodeEditor(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth();

    inline void enableHighlight(bool enable=true) {_highlightEnabled = enable;}

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;

    bool _highlightEnabled;
};



class LineNumberArea : public QWidget
{
public:
    LineNumberArea(GCodeEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    GCodeEditor *codeEditor;
};

#endif // GSHARPIE_GCODEEDITOR_H

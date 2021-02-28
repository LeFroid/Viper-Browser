/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QColor>
#include <QPlainTextEdit>

class LineNumberArea;

/**
 * @class CodeEditor
 * @brief Widget used in combination with a syntax highlighter in order
 *        to modify source code.
 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    /// Constructs the code editor with the given parent
    explicit CodeEditor(QWidget *parent = nullptr);

    /// Returns the width of the region of the code editor that displays line numbers
    int getLineNumberAreaWidth() const;

    /// Paints the line number area onto the code editor widget
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    /// Called when the size of the editor has been changed
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    /// Called when the block count has changed. Updates the line count area size as needed
    void onBlockCountChanged(int newBlockCount);

    /// Called when the viewport has been scrolled, updates the region displayed in the line number area
    void updateLineNumberArea(const QRect &rect, int dy);

    /// Highlights the line in which the cursor is active
    void highlightCurrentLine();

private:
    /// Widget used to display the line number on the code editor
    LineNumberArea *m_lineNumberArea;

    /// Color of the active line
    QColor m_colorCurrentLine;
};

/**
 * @class LineNumberArea
 * @brief Widget added to the side of a \ref CodeEditor in order to display line numbers
 */
class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    /// Constructs the line number area with the given parent
    explicit LineNumberArea(CodeEditor *parent);

    /// Returns the size value the widget should use
    QSize sizeHint() const override;

protected:
    /// Called when the line number area is painted onto its parent
    void paintEvent(QPaintEvent *event) override;

private:
    /// Direct pointer to parent widget
    CodeEditor *m_codeEditor;
};

#endif // CODEEDITOR_H

/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2006
 *
 *  Last modified:
 *     $Date: 2009-05-13 11:04:40 +0200 (Wed, 13 May 2009) $ by $Author: schulte $
 *     $Revision: 9083 $
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gecode/gist/nodewidget.hh>
#include <gecode/gist/drawingcursor.hh>

namespace Gecode { namespace Gist {

  NodeWidget::NodeWidget(NodeStatus s) : status(s) {
    setMinimumSize(22,22);
    setMaximumSize(22,22);
  }

  void NodeWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int hw= width()/2;
    int myx = hw+2; int myy = 2;
    switch (status) {
      case SOLVED:
        {
          QPoint points[4] = {QPoint(myx,myy),
                              QPoint(myx+8,myy+8),
                              QPoint(myx,myy+16),
                              QPoint(myx-8,myy+8)
                             };
          painter.setBrush(QBrush(DrawingCursor::green));
          painter.drawConvexPolygon(points, 4);
        }
        break;
      case FAILED:
        {
          painter.setBrush(QBrush(DrawingCursor::red));
          painter.drawRect(myx-6, myy+2, 12, 12);
        }
        break;
      case BRANCH:
        {
          painter.setBrush(QBrush(DrawingCursor::blue));
          painter.drawEllipse(myx-8, myy, 16, 16);
        }
        break;
      case UNDETERMINED:
        {
          painter.setBrush(QBrush(Qt::white));
          painter.drawEllipse(myx-8, myy, 16, 16);
        }
        break;
      default:
        break;
    }
  }

}}

// STATISTICS: gist-any

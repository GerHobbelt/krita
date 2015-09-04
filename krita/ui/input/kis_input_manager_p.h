/*
 *  Copyright (C) 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <QList>
#include <QPointer>
#include <QSet>
#include <QEvent>
#include <QTouchEvent>
#include <QScopedPointer>

#include "kis_input_manager.h"
#include "kis_shortcut_matcher.h"
#include "kis_tool_invocation_action.h"
#include "kis_alternate_invocation_action.h"
#include "kis_shortcut_configuration.h"
#include "kis_canvas2.h"
#include "kis_tool_proxy.h"
#include "kis_signal_compressor.h"
#include "kis_tablet_event.h"
#include "input/kis_tablet_debugger.h"


#define start_ignore_cursor_events() d->ignoreQtCursorEvents = true
#define stop_ignore_cursor_events() d->ignoreQtCursorEvents = false
#define break_if_should_ignore_cursor_events() if (d->ignoreQtCursorEvents) break;

#define push_and_stop_ignore_cursor_events() bool __saved_ignore_events = d->ignoreQtCursorEvents; d->ignoreQtCursorEvents = false
#define pop_ignore_cursor_events() d->ignoreQtCursorEvents = __saved_ignore_events

#define touch_start_block_press_events() d->touchHasBlockedPressEvents = d->disableTouchOnCanvas
#define touch_stop_block_press_events() d->touchHasBlockedPressEvents = false
#define break_if_touch_blocked_press_events() if (d->touchHasBlockedPressEvents) break;
#include "kis_abstract_input_action.h"


class Q_DECL_HIDDEN KisInputManager::Private
{
public:
    Private(KisInputManager *qq);
    bool tryHidePopupPalette();
    void saveTabletEvent(const QTabletEvent *event);
    void resetSavedTabletEvent(QEvent::Type type);
    void addStrokeShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, Qt::MouseButtons buttons);
    void addKeyShortcut(KisAbstractInputAction* action, int index,const QList<Qt::Key> &keys);
    void addTouchShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture );
    void addWheelShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, KisShortcutConfiguration::MouseWheelMovement wheelAction);
    bool processUnhandledEvent(QEvent *event);
    Qt::Key workaroundShiftAltMetaHell(const QKeyEvent *keyEvent);
    void setupActions();
    void saveTouchEvent( QTouchEvent* event );
    bool handleKisTabletEvent(QObject *object, KisTabletEvent *tevent);

    KisInputManager *q;

    KisCanvas2 *canvas;
    KisToolProxy *toolProxy;

    bool forwardAllEventsToTool;
    bool ignoreQtCursorEvents;

    bool disableTouchOnCanvas;
    bool touchHasBlockedPressEvents;

    KisShortcutMatcher matcher;
#ifdef HAVE_X11
    QPointF hiResEventsWorkaroundCoeff;
#endif
    QTabletEvent *lastTabletEvent;
    QTouchEvent *lastTouchEvent;

    KisToolInvocationAction *defaultInputAction;

    QObject *eventsReceiver;
    KisSignalCompressor moveEventCompressor;
    QScopedPointer<KisTabletEvent> compressedMoveEvent;
    bool testingAcceptCompressedTabletEvents;
    bool testingCompressBrushEvents;


    QSet<QPointer<QObject> > priorityEventFilter;


    template <class Event, bool useBlocking>
    void debugEvent(QEvent *event)
    {
      if (!KisTabletDebugger::instance()->debugEnabled()) return;
      QString msg1 = useBlocking && ignoreQtCursorEvents ? "[BLOCKED] " : "[       ]";
      Event *specificEvent = static_cast<Event*>(event);
      dbgKrita << KisTabletDebugger::instance()->eventToString(*specificEvent, msg1);
    }

    class ProximityNotifier : public QObject
    {
    public:
        ProximityNotifier(Private *_d, QObject *p);
        bool eventFilter(QObject* object, QEvent* event );
    private:
        KisInputManager::Private *d;
    };

    class CanvasSwitcher : public QObject
    {
    public:
        CanvasSwitcher(Private *_d, QObject *p);
        void addCanvas(KisCanvas2 *canvas);
        void removeCanvas(KisCanvas2 *canvas);
        bool eventFilter(QObject* object, QEvent* event );

    private:
        KisInputManager::Private *d;
        QMap<QObject*, KisCanvas2*> canvasResolver;
        int eatOneMouseStroke;
    };
    QPointer<CanvasSwitcher> canvasSwitcher;

    bool focusOnEnter;
    bool containsPointer;
};

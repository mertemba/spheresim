#ifndef _STATUSVIEWER_H_
#define _STATUSVIEWER_H_

#include <QObject>
#include <QTimer>
#include "NanosecondTimer.h"
#include "GLWidget.h"
#include "ClTimer.h"

class StatusViewer : public QThread, protected NanosecondTimer {

	Q_OBJECT // must include this if you use Qt signals/slots	  

protected:
	GLWidget* glWidget;
	ClTimer* clTimer;
	double lastGlWidgetFrames, lastClTimerFrames;
	const static double f = 0.5;

public:
	StatusViewer(GLWidget* glw, ClTimer* clt);
	
	void run();

};


#endif  /* _STATUSVIEWER_H_ */

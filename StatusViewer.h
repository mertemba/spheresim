#ifndef _STATUSVIEWER_H_
#define _STATUSVIEWER_H_

//#include <QObject>
//#include <QTimer>
#include <QThread>
#include "NanosecondTimer.h"
#include "Circles.h"
class GLWidget;
class ClTimer;

class StatusViewer : public QThread, protected NanosecondTimer {

	Q_OBJECT // must include this if you use Qt signals/slots	  

protected:
	GLWidget* glWidget;
	ClTimer* clTimer;
	scalar lastGlWidgetFrames, lastClTimerFrames;
	const static scalar f = 0.5;

public:
	StatusViewer(GLWidget* glw, ClTimer* clt);
	
	void run();

};


#endif  /* _STATUSVIEWER_H_ */

#include <QVector>
#include <QMainWindow>
#include <QLabel>
#include <QGridLayout>
#include <QApplication>
#include <QPixmap>
#include <QPushButton>
#include <QPalette>
#include <QPlainTextEdit>
#include <QDebug>
#include <QCheckBox>
#include <QPainter>
#include <QPen>
#include <QLine>
#include <QPoint>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

class WCwindow : public QMainWindow
{
	Q_OBJECT
public:
	WCwindow();
	bool eventFilter(QObject *obj, QEvent *event);
	void drawCircle(int x, int y, int radius, QRgb color);
	void drawCircle2(int x, int y, int radius, QRgb color);
	void drawSegment(int x, int y, int radius, QRgb color);
	void drawSegment2(int x, int y, int diameter, QRgb color);
	void textOut(QString text1);

	QGridLayout *layout;
	QGridLayout *rightSidePanel;
	QImage *image;
	QLabel *imageHolder;
	QPlainTextEdit *text;
	QPushButton *buttonSend;
	QPushButton *buttonClear;
	QPushButton *buttonSave;
	QPushButton *buttonLoad;
	QCheckBox *checkBoxRed;
	QCheckBox *checkBoxGreen;
	QCheckBox *checkBoxBlue;
	QRgb pictureDataAVR[128][16];
	QRgb activeColor = qRgb(100, 100, 100);
	int ledPixelCount = 16;
	int imageSize = (ledPixelCount+1) * 31;
	QLineF *mouseMovementLine = new QLineF();

public slots:
	void handleButtonSend();
	void handleButtonClear();
	void handleButtonSave();
	void handleButtonLoad();
	void handleCheckBoxes(int state);
};

WCwindow::WCwindow()
{
	buttonSend = new QPushButton("Send", this);
	buttonClear = new QPushButton("Clear", this);
	buttonSave = new QPushButton("Save", this);
	buttonLoad = new QPushButton("Load", this);
	image = new QImage(imageSize, imageSize, QImage::Format_ARGB32);
	layout = new QGridLayout(this);
	rightSidePanel = new QGridLayout(this);
	text = new QPlainTextEdit(this);
	imageHolder = new QLabel(this);
	checkBoxRed = new QCheckBox("Red", this);
	checkBoxGreen = new QCheckBox("Green", this);
	checkBoxBlue = new QCheckBox("Blue", this);

	connect(buttonSend, SIGNAL(released()), this, SLOT(handleButtonSend()));
	connect(buttonClear, SIGNAL(released()), this, SLOT(handleButtonClear()));
	connect(buttonSave, SIGNAL(released()), this, SLOT(handleButtonSave()));
	connect(buttonLoad, SIGNAL(released()), this, SLOT(handleButtonLoad()));
	connect(checkBoxRed, SIGNAL(stateChanged(int)), this, SLOT(handleCheckBoxes(int)));
	connect(checkBoxGreen, SIGNAL(stateChanged(int)), this, SLOT(handleCheckBoxes(int)));
	connect(checkBoxBlue, SIGNAL(stateChanged(int)), this, SLOT(handleCheckBoxes(int)));

	text->setReadOnly(true);
	text->insertPlainText("Testing\n");
	text->setMinimumHeight(200);
	
	for (int x = 0; x < image->width(); ++x) {
		for (int y = 0; y < image->height(); ++y) {
			if ((x - imageSize / 2)*(x - imageSize / 2) + (y - imageSize / 2)*(y - imageSize / 2) <= (ledPixelCount * 16)*(ledPixelCount * 16)) {
				image->setPixel(x, y, qRgba(0, 0, 0, 255));
			}
			else {
				image->setPixel(x, y, qRgba(0, 0, 0, 0));
			}
		}
	}

	imageHolder->setPixmap(QPixmap::fromImage(*image));

	layout->addWidget(imageHolder, 0, 0);
	layout->addWidget(text, 2, 0, 1, 2);
	layout->addLayout(rightSidePanel, 0, 1);

	rightSidePanel->addWidget(checkBoxRed, 1, 0);
	rightSidePanel->addWidget(checkBoxGreen, 2, 0);
	rightSidePanel->addWidget(checkBoxBlue, 3, 0);
	rightSidePanel->addWidget(buttonClear, 5, 0);
	rightSidePanel->addWidget(buttonSend, 7, 0);
	rightSidePanel->addWidget(buttonSave, 9, 0);
	rightSidePanel->addWidget(buttonLoad, 10, 0);
	rightSidePanel->setRowStretch(0, 1);
	rightSidePanel->setRowStretch(4, 4);
	rightSidePanel->setRowStretch(6, 4);
	rightSidePanel->setRowStretch(8, 4);
	rightSidePanel->setRowStretch(11, 4);
	
	QWidget *central = new QWidget();
	setCentralWidget(central);
	centralWidget()->setLayout(layout);

	setWindowTitle("Pildigeneraator");
	imageHolder->setMouseTracking(true);
	imageHolder->installEventFilter(this);

}

void WCwindow::handleButtonSend() {
	textOut(QString("Send\n"));
}
void WCwindow::handleButtonClear() {
	//clear the image
	image->fill(QColor(0, 0, 0, 0));
	QPainter painter(image);
	painter.setPen(QPen(QColor(0,0,0,255), ledPixelCount*16*2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	painter.drawPoint(imageSize / 2, imageSize / 2);
	
	imageHolder->setPixmap(QPixmap::fromImage(*image));
}
void WCwindow::handleButtonSave() {
	textOut(QString("Save\n"));
}
void WCwindow::handleButtonLoad() {
	textOut(QString("Load\n"));
}
void WCwindow::handleCheckBoxes(int state) {
	//change the color according to the checkboxes
	QColor colorNew(0,0,0);
	if (checkBoxRed->isChecked()) {
		colorNew.setRed(255);
	}
	if (checkBoxGreen->isChecked()) {
		colorNew.setGreen(255);
	}
	if (checkBoxBlue->isChecked()) {
		colorNew.setBlue(255);
	}
	activeColor = colorNew.rgb();
}

bool WCwindow::eventFilter(QObject *obj, QEvent *event) {
	if (qobject_cast<QLabel*>(obj) == imageHolder) {
		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::MouseMove) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->buttons() & Qt::LeftButton) {
				if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
					mouseMovementLine->setP2(mouseEvent->pos());
					drawSegment2(mouseMovementLine->x2(), mouseMovementLine->y2(), 7, activeColor);
					imageHolder->setPixmap(QPixmap::fromImage(*image));
				}
				else {
					mouseMovementLine->setP1(mouseMovementLine->p2());
					mouseMovementLine->setP2(mouseEvent->pos());
					for (float t = 0; t <= 1; t += 1 / mouseMovementLine->length()) {
						QPointF point = mouseMovementLine->pointAt(t);
						drawSegment2(point.x(), point.y(), 7, activeColor);
					}
					imageHolder->setPixmap(QPixmap::fromImage(*image));
				}

				//textOut(QString("Coord:%1, %2\n").arg(QString::number(mouseEvent->pos().x()), QString::number(mouseEvent->pos().y())));
				//drawSegment2(x, y, 7, qRgb(100, 100, 100));
			}
		}
	}
	return false;
}

void WCwindow::textOut(QString text1) {
	text->moveCursor(QTextCursor::End);
	text->insertPlainText(text1); //, this->objectName, obj->objectName)
	text->ensureCursorVisible();
}

void WCwindow::drawSegment2(int x, int y, int diameter, QRgb color) {
	//paints a segment corresponding to the point x,y, only changes the image file, doesn't display it
	int x1 = x - imageSize / 2;
	int y1 = y - imageSize / 2; //x1 and y1 begin at the center and grow left and up
	float angle = atan2f(y1, x1); //0 deg is on the x axis and goes clockwise
	float distance = sqrtf(x1*x1 + y1*y1);
	int ledNumberFromTheCenter = roundf(distance / ledPixelCount);
	int segmentNumber = roundf(64 * angle / (M_PI));
	segmentNumber = segmentNumber < 0 ? 128 + segmentNumber : segmentNumber;
	//textOut(QString("LED2: %1, segment: %2\n").arg(QString::number(ledNumberFromTheCenter), QString::number(segmentNumber)));
	
	if (ledNumberFromTheCenter < 16 && color != pictureDataAVR[segmentNumber][ledNumberFromTheCenter]) {
		//if the color is different and the point isn't out of the display, paint it
		QRectF rect(imageSize / 2 - ledNumberFromTheCenter*ledPixelCount, imageSize / 2 - ledNumberFromTheCenter*ledPixelCount,
			2 * ledNumberFromTheCenter*ledPixelCount, 2 * ledNumberFromTheCenter*ledPixelCount);
		QPainter painter(image);
		painter.setPen(QPen(QColor(color), diameter, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		pictureDataAVR[segmentNumber][ledNumberFromTheCenter] = color;
		if (ledNumberFromTheCenter == 0) {
			painter.drawPoint(imageSize / 2, imageSize / 2);
		}
		else {
			painter.drawArc(rect, -16 * segmentNumber * 360 / 128, 16 * 360 / 128);
		}
	}
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	WCwindow win;
	win.show();
	return app.exec();
}

#include "main.moc"
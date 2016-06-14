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
#include <QFileDialog>
#include <QColor>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include "Serial.h"

int qRed(QRgb rgb);
int qGreen(QRgb rgb);
int qBlue(QRgb rgb);

class WCwindow : public QMainWindow
{
	Q_OBJECT
public:
	WCwindow();
	bool eventFilter(QObject *obj, QEvent *event);
	void drawSegmentXY(int x, int y, int diameter, QRgb color, Qt::PenCapStyle = Qt::RoundCap);
	void drawSegment(int angleStep, int lengthStep, int diameter, QRgb color, Qt::PenCapStyle penCapStyle = Qt::RoundCap);
	void textOut(QString text1);
	void clearImageNoPaint();
	void clearAVRData();
	int WCwindow::getColorBit(QRgb col, int colornum);
	void WCwindow::getUSBCommand(char* destination);

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
	QRgb pictureDataAVR[128][16] = {};
	QRgb activeColor;
	int ledPixelCount = 16;
	int imageSize = ledPixelCount * 32;
	QLineF *mouseMovementLine = new QLineF();
	int imageLEDLightDiameter = 7;
	CSerial serial;

public slots:
	void handleButtonSend();
	void handleButtonClear();
	void handleButtonSave();
	void handleButtonLoad();
	void handleCheckBoxes(int state);
};

WCwindow::WCwindow()
{
	//make the window, make all the GUI elements, put them in layouts, register event handlers
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
	
	handleButtonClear();

	checkBoxRed->setChecked(false);
	checkBoxGreen->setChecked(true);
	checkBoxBlue->setChecked(false);

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

int WCwindow::getColorBit(QRgb col, int colorNum) {
	//used in generating the data that the microcontroller wants, returns whether the given color is not zero
	int returnValue;
	if (colorNum == 0) {
		returnValue = qRed(col);
	}
	else if (colorNum == 1) {
		returnValue = qGreen(col);
	}
	else if (colorNum == 2) {
		returnValue = qBlue(col);
	}
	if (returnValue) {
		return 1;
	}
	else {
		return 0;
	}
}

void WCwindow::getUSBCommand(char* destination) {
	//reads up to newline from USB
	while (1) {
		while (!serial.ReadDataWaiting());
		serial.ReadData(destination, 1);
		++destination;
		if (*(destination-1) == '\n') {
			*destination = 0;
			return;
		}
	}
}

void WCwindow::handleButtonSend() {
	//generates the data to send to the microcontroller and sends it, avrData is in the same format as the microcontroller uses
	//the picturedata is in the pictureDataAVR array, but it is in a different format, because it has a QRgb for every 16 leds of the 128 segments,
	//but the microcontroller wants for each segment a 6 byte value with each bit in the 6 bytes representing each led
	textOut("Generating data\n");
	char avrData [128][6] = { 0 };
	for (int n = 0; n < 128; ++n) {
		for (int m = 0; m < 3; ++m) {
			for (int k = 0; k < 16; ++k) {
				avrData[n][m + (k >= 8) * 3] |= getColorBit(pictureDataAVR[n][k], m)<<(k%8);
			}
		}
	}
	textOut("Sending data\n");
	if (serial.Open(3, 115200)) {
		serial.SendData((char*)avrData, 6*128);
		char data[64] = { 0 };
		while (1) {
			//wait for the Eeprom to be written
			getUSBCommand(data);
			textOut(data);
			if (strcmp(data, "Eeprom written\n") == 0) {
				break;
			}
		}
		textOut("Sending end.\n\n");
		serial.Close();
	}
	else {
		textOut("COM port not open.\n");
	}
}
void WCwindow::clearAVRData() {
	//for zeroing the array with the segment data
	for (int ang = 0; ang < 128; ++ang) {
		for (int led = 0; led < 16; ++led) {
			pictureDataAVR[ang][led] = qRgb(0, 0, 0);
		}
	}
}
void WCwindow::clearImageNoPaint() {
	//clear the image
	image->fill(QColor(0, 0, 0, 0));
	QPainter painter(image);
	painter.setPen(QPen(QColor(0, 0, 0, 255), imageSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	painter.drawPoint(imageSize / 2, imageSize / 2);
}
void WCwindow::handleButtonClear() {
	//clear the image and display the cleared image
	clearImageNoPaint();
	clearAVRData();
	imageHolder->setPixmap(QPixmap::fromImage(*image));
}
void WCwindow::handleButtonSave() {
	//save the current image to an image file
	QString filename = QFileDialog::getSaveFileName(this, "Select the image file", "", "Images (*.png *.jpg *.jpeg *.bmp)");
	if (!filename.isEmpty()) {
		//make a new image, where we draw with a flat cap, to minimize rounding errors
		QImage image2 = *image;
		clearImageNoPaint();
		for (int angleStep = 0; angleStep < 128; ++angleStep) {
			for (int lengthStep = 0; lengthStep < 16; ++lengthStep) {
				QRgb col = pictureDataAVR[angleStep][lengthStep];
				if (col != qRgb(0, 0, 0)) {
					//if we would use a round cap instead of a flat cap, it would add half a sphere to the ends of the segments and color neighboring segments
					drawSegment(angleStep, lengthStep, imageLEDLightDiameter, col, Qt::FlatCap);
				}
			}
		}
		image->save(filename, (const char*)nullptr, 100);
		*image = image2;
	}
}
void WCwindow::handleButtonLoad() {
	//load an image from a file
	QString filename = QFileDialog::getOpenFileName(this, "Open the image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
	
	if (!filename.isEmpty()) {
		clearImageNoPaint();
		clearAVRData();
		QImage image2;
		image2.load(filename);
		int size = image2.height() < image2.width() ? image2.height() : image2.width(); //choose the smaller side and make a square with it
		for (int angleStep = 0; angleStep < 128; ++angleStep) {
			for (int lengthStep = 0; lengthStep < 16; ++lengthStep) {
				//for each angle and led take the corresponding pixel from the image and set the colors based on whether the pixel colors are over 255/2
				int x = size / 2 + lengthStep * cosf(angleStep * 2 * M_PI / 128) * size / 2 / 16;
				int y = size / 2 + lengthStep * sinf(angleStep * 2 * M_PI / 128) * size / 2 / 16;
				QColor color = QColor(image2.pixel(x, y));
				color.setRed(color.red() > 255 / 2 ? 255 : 0);
				color.setGreen(color.green() > 255 / 2 ? 255 : 0);
				color.setBlue(color.blue() > 255 / 2 ? 255 : 0);
				if (color.rgb() != qRgb(0, 0, 0)){
					//if the color was black, don't draw, because you will cover neighboring non-black segments, because the drawSegment function draws a little bit over the segment
					drawSegment(angleStep, lengthStep, imageLEDLightDiameter, color.rgb());
				}
			}
		}
		imageHolder->setPixmap(QPixmap::fromImage(*image));
	}
}
void WCwindow::handleCheckBoxes(int state) {
	//change the current drawing color according to the checkboxes
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
	//this function draws segments on the screen in response to mouse dragging and clicking
	if (qobject_cast<QLabel*>(obj) == imageHolder) {
		if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::MouseMove) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->buttons() & Qt::LeftButton) {
				if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
					//the mouse was clicked, so remember the last place where we clicked in the mouseMovementLine line
					//this is needed because if we drag very fast, some points might get missed, so we keep in mind the last point and go through all the points in between by ourselves
					mouseMovementLine->setP2(mouseEvent->pos());
					drawSegmentXY(mouseMovementLine->x2(), mouseMovementLine->y2(), imageLEDLightDiameter, activeColor);
					imageHolder->setPixmap(QPixmap::fromImage(*image));
				}
				else {
					//the mouse was dragged while the button was down, make a line connecting the last and the current point and go through all the points inbetween and color the corresponding segments
					mouseMovementLine->setP1(mouseMovementLine->p2());
					mouseMovementLine->setP2(mouseEvent->pos());
					for (float t = 0; t <= 1; t += 1 / mouseMovementLine->length()) {
						QPointF point = mouseMovementLine->pointAt(t);
						drawSegmentXY(point.x(), point.y(), imageLEDLightDiameter, activeColor);
					}
					imageHolder->setPixmap(QPixmap::fromImage(*image));
				}
			}
		}
	}
	return false;
}

void WCwindow::textOut(QString text1) {
	//simply puts text to the big textedit
	text->moveCursor(QTextCursor::End);
	text->insertPlainText(text1);
	text->ensureCursorVisible();
}

void WCwindow::drawSegment(int angleStep, int lengthStep, int diameter, QRgb color, Qt::PenCapStyle penCapStyle) {
	//paints the given segment the given color
	QRectF rect(imageSize / 2 - lengthStep*ledPixelCount, imageSize / 2 - lengthStep*ledPixelCount,
		2 * lengthStep*ledPixelCount, 2 * lengthStep*ledPixelCount);
	QPainter painter(image);
	painter.setPen(QPen(QColor(color), diameter, Qt::SolidLine, penCapStyle, Qt::RoundJoin));
	pictureDataAVR[angleStep][lengthStep] = color;
	if (lengthStep == 0) {
		//if it is the first led in the middle, just draw a point
		painter.drawPoint(imageSize / 2, imageSize / 2);
	}
	else {
		//draw an arc representing the segment, rotate the segment by half a step, so that the midpoint is given by the angle in polar coordinates
		painter.drawArc(rect, -16 * (angleStep + 0.5) * 360 / 128, 16 * 360 / 128);
	}
	if (color == qRgb(0, 0, 0)) {
		//if we draw a black segment, redraw its neighboring segments so that we wont have a black segment covering colored segments on the sides, but the other way around
		painter.end(); //needed so that we can draw again the old colors
		int angleStep2 = (angleStep + 1) % 128;
		QRgb col = pictureDataAVR[angleStep2][lengthStep];
		if (col != qRgb(0, 0, 0)) {
			drawSegment(angleStep2, lengthStep, diameter, col, penCapStyle);
		}
		angleStep2 = (angleStep + 128 - 1) % 128;
		col = pictureDataAVR[angleStep2][lengthStep];
		if (col != qRgb(0, 0, 0)) {
			drawSegment(angleStep2, lengthStep, diameter, col, penCapStyle);
		}
	}
}

void WCwindow::drawSegmentXY(int x, int y, int diameter, QRgb color, Qt::PenCapStyle penCapStyle) {
	//calculates the sgement where the point belongs to and paints it
	int x1 = x - imageSize / 2;
	int y1 = y - imageSize / 2; //x1 and y1 begin at the center and grow left and up
	float angle = atan2f(y1, x1); //0 deg is on the x axis and goes clockwise
	float distance = sqrtf(x1*x1 + y1*y1);
	int ledNumberFromTheCenter = roundf(distance / ledPixelCount);
	int segmentNumber = roundf(64 * angle / (M_PI));
	segmentNumber = segmentNumber < 0 ? 128 + segmentNumber : segmentNumber;
	
	if (ledNumberFromTheCenter < 16) {
		drawSegment(segmentNumber, ledNumberFromTheCenter, diameter, color, penCapStyle);
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
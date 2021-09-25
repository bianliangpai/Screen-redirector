#include <windows.h>
#include <wingdi.h>

#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QtWidgets>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

BITMAPINFOHEADER CreateBitmapHeader(int width, int height) {
  BITMAPINFOHEADER bi;

  // create a bitmap
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = width;
  bi.biHeight =
      -height;  // this is the line that makes it draw upside down or not
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  return bi;
}

class Mat {
 public:
  Mat() : _w{0}, _h{0} {}

  void create(int h, int w) {
    _h = h;
    _w = w;

    _data.clear();
    _data.resize(h * w * 4);
  }

  int w() const { return _w; }
  int h() const { return _h; }

  uchar operator[](std::size_t offset) const { return _data[offset]; }
  uchar* data() {
    if (_data.size() > 0) {
      return &_data[0];
    } else {
      return nullptr;
    }
  }

 private:
  int _h, _w;
  std::vector<uchar> _data;
};

Mat captureScreenMat(HWND hwnd) {
  Mat src;

  // get handles to a device context (DC)
  HDC hwindowDC = GetDC(hwnd);
  HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
  SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

  // define scale, height and width
  int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int dstwidth = width / 2;
  int dstheight = height / 2;

  // create mat object
  src.create(dstheight, dstwidth);

  // create a bitmap
  HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, dstwidth, dstheight);
  BITMAPINFOHEADER bi = CreateBitmapHeader(dstwidth, dstheight);

  // use the previously created device context with the bitmap
  SelectObject(hwindowCompatibleDC, hbwindow);

  // copy from the window device context to the bitmap device context
  StretchBlt(hwindowCompatibleDC, 0, 0, dstwidth, dstheight, hwindowDC, screenx,
             screeny, width, height,
             SRCCOPY);  // change SRCCOPY to NOTSRCCOPY for wacky colors !

  // copy from hwindowCompatibleDC to hbwindow
  GetDIBits(hwindowCompatibleDC, hbwindow, 0, dstheight, src.data(),
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);

  // avoid memory leak
  DeleteObject(hbwindow);
  DeleteDC(hwindowCompatibleDC);
  ReleaseDC(hwnd, hwindowDC);

  return src;
}

void func(std::function<void()> f) { f(); }

class MyWindow : public QMainWindow {
 public:
  MyWindow() : QMainWindow{}, _label{nullptr} {
    this->setWindowTitle("MyWindow");

    _label = new QLabel(this);
    setCentralWidget(_label);

    auto logic = [this]() {
      while (true) {
        HWND hwnd = GetDesktopWindow();
        Mat src = captureScreenMat(hwnd);
        QImage qi{src.data(), src.w(), src.h(), src.w() * 4,
                  QImage::Format_RGBA8888};
        QPixmap pm = QPixmap::fromImage(qi);

        if (_label) {
          _label->setPixmap(pm);
        } else {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
      }
    };
    _t = std::thread(func, logic);
  }

  ~MyWindow() {
    delete _label;
    _label = nullptr;

    if (_t.joinable()) {
      _t.join();
    }
  }

 private:
  QLabel* _label;
  std::thread _t;
};

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  MyWindow window;
  window.show();

  return app.exec();
}
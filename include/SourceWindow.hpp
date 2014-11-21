#include "GtkPP.hpp"

class SourceWindow: public GtkPP::ScrolledWindow
{
public:
  SourceWindow()
  {
    GtkPP::ScrolledWindow::Init();

    auto sview = gtk_source_view_new();
    Add(sview);

    ShowAll();
  }
  SourceWindow(const SourceWindow& s):Widget(s) { }
};

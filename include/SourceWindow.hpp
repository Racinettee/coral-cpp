#include "GtkPP.hpp"

class SourceWindow: public GtkPP::ScrolledWindow
{
public:
  SourceWindow()
  {
    auto sview = gtk_source_view_new();
    Add(sview);

    ShowAll();
  }
};

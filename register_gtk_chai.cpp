#include <gtk/gtk.h>
// -----------------
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/utility/utility.hpp>
// --------------------------------------
#include <gtksourceview/gtksource.h>
#include "include/SourceWindow.hpp"

GtkWidget* copy_widget_pointer(GtkWidget* other)
{
  return GTK_WIDGET(other);
}
void register_gtk_chai(chaiscript::ChaiScript& chai)
{
  using namespace GtkPP;
  using namespace chaiscript;

  auto m = ModulePtr(new Module);

  utility::add_class<Widget>(*m,
    "GtkWidget",
    {
      constructor<Widget()>(),
      constructor<Widget(const Widget&)>()
    },
    {
      {fun(&Widget::Show), "Show"},
      {fun(&Widget::Hide), "Hide"},
      //{fun(gtk_widget_activate), "activate"},
      {fun(&Widget::Name), "Name"},
      {fun(&Widget::operator=), "="}
    }
  );
  chai.add(m);

  m = ModulePtr(new Module);

  utility::add_class<Container>(*m,
    "GtkContainer",
    {
    },
    {
      {fun(&Container::Add), "Add"},
      {fun(&Container::Remove), "Remove"}
    }
  );

  chai.add(m);

  chai.add(base_class<Widget, Container>());

  m = ModulePtr(new Module);

  utility::add_class<ScrolledWindow>(*m,
    "GtkScrolledWindow",
    {
      constructor<ScrolledWindow()>()
    },
    {
      {fun(&ScrolledWindow::GetChild), "Child"}
    }
  );
  chai.add(m);

  chai.add(base_class<Container,ScrolledWindow>());

  m = ModulePtr(new Module);

  utility::add_class<SourceWindow>(*m,
    "Sourcewindow",
    {
      constructor<SourceWindow()>()
    },
    {
      {fun(&SourceWindow::GetChild), "SrcView"}
    }
  );
  chai.add(m);

  chai.add(base_class<ScrolledWindow, SourceWindow>());

  m = ModulePtr(new Module);

  utility::add_class<Builder>(*m,
    "Builder",
    {
    },
    {
      {fun(&Builder::GetWidget), "GetWidget"}
    }
  );
  chai.add(m);
}

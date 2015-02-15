#include <gtk/gtk.h>
// -----------------
#include "chaiscript/chaiscript.hpp"
#include "chaiscript/utility/utility.hpp"
// --------------------------------------
#include <gtksourceview/gtksource.h>
#include "SourceWindow.hpp"

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
      {fun(&Widget::Activate), "Activate"},
      {fun(&Widget::Name), "Name"},
      {fun(&Widget::operator=), "="},
      {fun(AsNotebook), "AsNotebook"},
      {fun(AsContainer), "AsContainer"},
      {fun(AsScrolledWindow), "AsScrolledWindow"}
    }
  );
  //chai.add(m);

  //m = ModulePtr(new Module);

  utility::add_class<Container>(*m,
    "GtkContainer",
    {
    },
    {
      {fun(&Container::Add), "Add"},
      {fun(&Container::Remove), "Remove"}
    }
  );

  //chai.add(m);

  //m = ModulePtr(new Module);

  utility::add_class<ScrolledWindow>(*m,
    "GtkScrolledWindow",
    {
      constructor<ScrolledWindow()>()
    },
    {
      {fun(&ScrolledWindow::GetChild), "Child"}
    }
  );
  //chai.add(m);

  //m = ModulePtr(new Module);

  utility::add_class<SourceWindow>(*m,
    "Sourcewindow",
    {
      constructor<SourceWindow()>(),
      constructor<SourceWindow(const SourceWindow&)>()
    },
    {
      {fun(&SourceWindow::GetChild), "SrcView"}
    }
  );
  //chai.add(m);

  //m = ModulePtr(new Module);

  utility::add_class<Builder>(*m,
    "GtkBuilder",
    {
    },
    {
      {fun(&Builder::GetWidget), "GetWidget"}
    }
  );
  //chai.add(m);

  //auto n = ModulePtr(new Module);

  utility::add_class<Notebook>(*m,
    "GtkNotebook",
    {
      constructor<Notebook()>(),
      constructor<Notebook(const Notebook&)>()
    },
    {
      {fun(&Notebook::AppendPage), "AppendPage"},
      {fun<int(Notebook::*)()>(&Notebook::CurrentPage), "CurrentPage"},
      {fun<void(Notebook::*)(int)>(&Notebook::CurrentPage), "CurrentPage"},
      {fun(&Notebook::GetNthPage), "GetNthPage"},
      {fun(&Notebook::ShowTabs), "ShowTabs"},
      {fun(&Notebook::NPages), "NPages"},
      {fun(&Notebook::GetScrollable), "GetScrollable"},
      {fun(&Notebook::SetScrollable), "SetScrollable"}
    }
  );
  chai.add(m);

  chai.add(chaiscript::base_class<Widget, Container>());
  chai.add(chaiscript::base_class<Widget, Notebook>());
  chai.add(chaiscript::base_class<Container, Notebook>());
  chai.add(chaiscript::base_class<Container,ScrolledWindow>());
  chai.add(chaiscript::base_class<Widget, SourceWindow>());
  chai.add(chaiscript::base_class<ScrolledWindow, SourceWindow>());
}

#ifndef _GTKPP_
#define _GTKPP_

#if defined(DEBUG) || defined(_DEBUG)
    #ifndef DBG_ONLY
      #define DBG_ONLY(...) do { __VA_ARGS__; } while (0)
    #endif
#else
    #ifndef DBG_ONLY
      #define DBG_ONLY(...)
    #endif
#endif

#include <sstream>

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

/// GtkPP does not deal with reference counting automatically atm
namespace GtkPP
{
  class Notebook;
  class ScrolledWindow;

  class Widget
  {
  public:
    GtkWidget* w;
    // -----------
    void Show()
    {
      gtk_widget_show(GTK_WIDGET(w));
    }
    void ShowAll()
    {
      gtk_widget_show_all(GTK_WIDGET(w));
    }
    void Hide()
    {
      gtk_widget_hide(GTK_WIDGET(w));
    }
    const std::string Name()
    {
      std::ostringstream str("");
      str << std::string(gtk_widget_get_name(GTK_WIDGET(w))) << " " << this << ":" << w;
      return std::string(str.str());
    }
    /// Gtk documentation states this method works
    /// only on widgets that can be activated
    int Activate()
    {
      return gtk_widget_activate(GTK_WIDGET(w));
    }
    operator GtkWidget*()
    {
      return GTK_WIDGET(w);
    }
    Widget() { }
    Widget(const Widget& other):w(GTK_WIDGET(other.w)) { }
    Widget(GtkWidget* o): w(o) { }
    virtual ~Widget() {}
    // Constructor for use in chai
    virtual void Init()
    {
    }
  };
  class Container: public virtual Widget
  {
  public:
    Container() = default;
    Container(const Widget& o): Widget(o) { }
    void Add(Widget o)
    {
      gtk_container_add(GTK_CONTAINER(w), o);
    }
    void Remove(Widget o)
    {
      gtk_container_remove(GTK_CONTAINER(w), o);
    }
  };
  class ScrolledWindow: public virtual Container
  {
  public:
    Widget GetChild()
    {
      return Widget(gtk_bin_get_child(GTK_BIN(w)));
    }
    ScrolledWindow() { }
    ScrolledWindow(const Widget& n): Widget(n) { }
    virtual void Init() final
    {
      w = gtk_scrolled_window_new(0, 0);
      gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_FILL);
      gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_FILL);
    }
  };
  class Notebook: public virtual Container
  {
  public:
    Notebook() { }
    Notebook(const Notebook& n): Widget(n) { }
    Notebook(const Widget& n): Widget(n) { }
    int AppendPage(Widget o)
    {
      return (int)gtk_notebook_append_page(
        GTK_NOTEBOOK(w), o, nullptr);
    }
    int CurrentPage()
    {
      return (int)gtk_notebook_get_current_page(GTK_NOTEBOOK(w));
    }
    void CurrentPage(int n)
    {
      gtk_notebook_set_current_page(
        GTK_NOTEBOOK(w), (gint)n);
    }
    Widget GetNthPage(int n)
    {
      return gtk_notebook_get_nth_page(GTK_NOTEBOOK(w), n);
    }
    void ShowTabs(bool yon)
    {
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w),(gboolean)yon);
    }
    int NPages()
    {
      return (int)gtk_notebook_get_n_pages(GTK_NOTEBOOK(w));
    }
    // TODO get tab label
    bool GetScrollable()
    {
      return (bool)gtk_notebook_get_scrollable(GTK_NOTEBOOK(w));
    }
    void SetScrollable(bool yon)
    {
      gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), (gboolean)yon);
    }
  };
  class TextBuffer
  {
    GtkTextBuffer* b;
  public:
    TextBuffer() = default;
    TextBuffer(const TextBuffer& o):
      b(o.b) { }
    TextBuffer(GtkTextBuffer* o):
      b(o) { }
    operator GtkTextBuffer*()
    {
      return b;
    }
  };
  class TextView: public virtual Container
  {
  public:
    void Init()
    {
      w = gtk_text_view_new();
    }
    TextBuffer Buffer()
    {
      return gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(w));
    }
    void Buffer(TextBuffer o)
    {
      gtk_text_view_set_buffer(
        GTK_TEXT_VIEW(w), o);
    }
  };
  class Builder
  {
    GtkBuilder* b;
  public:
    Builder() { }
    Builder(GtkBuilder* builder): b(builder)  { }
    Builder(const std::string& file)
    {
      b= gtk_builder_new_from_file(file.c_str());
    }
    Widget* GetWidget(const std::string& n)
    {
      DBG_ONLY(
        std::cout << GTK_WIDGET(gtk_builder_get_object(b, n.c_str())) << std::endl;
      );
      auto r =  new Widget(GTK_WIDGET(gtk_builder_get_object(b, n.c_str())));
      DBG_ONLY( std::cout << r << std::endl; );
      return r;
    }
    operator GtkBuilder*()
    {
      return b;
    }
  };
  inline Notebook AsNotebook(const Widget& o)
  {
    return Notebook(o);
  }
  inline ScrolledWindow AsScrolledWindow(const Widget& o)
  {
    return ScrolledWindow(o);
  }
  inline Container AsContainer(const Widget& o)
  {
    return Container(o);
  }
};

#endif // _GTKPP_

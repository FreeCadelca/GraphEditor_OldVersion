#include "gtkmm.h"
#include "cairomm/context.h"
#include <cstdlib>
#include <string>
#include <cmath>
#include <map>
#include "Canvas.h"

int main(int argc, char **argv) {
    auto app = Gtk::Application::create(argc, argv);
    auto ui = Gtk::Builder::create_from_file("design.glade");//подключение дизайна

    Gtk::ApplicationWindow *window;//выцепление виджета окна
    ui->get_widget("window", window);

    Gtk::Label *printed_graph_label;//poluchenie виджета лейбла с распечаткой
    ui->get_widget("printed_graph_label", printed_graph_label);

    Canvas *canvas = new Canvas(printed_graph_label);//создание холста
    Gtk::Box *box;//выцепление виджета основной коробки
    ui->get_widget("box", box);
    box->add(*canvas);
    canvas->show();

    Gtk::ToolButton *tool_choose_color;//выцепление виджета инструмента выбора цвета
    ui->get_widget("tool_choose_color", tool_choose_color);
    tool_choose_color->signal_clicked().connect(sigc::mem_fun(*canvas, &Canvas::choose_color));
    //подключение функционала при нажатии

    Gtk::ToolButton *tool_add_vertex;//выцепление виджета инструмента рисования вершин
    ui->get_widget("tool_add_vertex", tool_add_vertex);
    tool_add_vertex->signal_clicked().connect(//подключение функционала при нажатии
            sigc::bind(
                    sigc::mem_fun(
                            *canvas,
                            &Canvas::change_tool
                    ),
                    Canvas::VERTEX
            )
    );


    Gtk::ToolButton *tool_add_edge;//выцепление виджета инструмента рисования рёбер
    ui->get_widget("tool_add_edge", tool_add_edge);
    tool_add_edge->signal_clicked().connect(//подключение функционала при нажатии
            sigc::bind(
                    sigc::mem_fun(
                            *canvas,
                            &Canvas::change_tool
                    ),
                    Canvas::EDGE
            )
    );

    Gtk::Button *print_grapf_button;//выцепление виджета кнопки разворачивания распечатки графа
    ui->get_widget("print_graph_button", print_grapf_button);
    print_grapf_button->signal_clicked().connect(//подключение функционала при нажатии
            sigc::bind(
                    sigc::mem_fun(
                            *canvas,
                            &Canvas::print_graph
                    ),
                    print_grapf_button
            )

    );

    return app->run(*window);//вызов окна
}
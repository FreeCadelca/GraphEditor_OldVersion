//
// Created by Dmitriy on 02.04.2024.
//

#ifndef UNTITLED_CANVAS_H
#define UNTITLED_CANVAS_H

#include "gtkmm.h"
#include "cairomm/context.h"
#include <cstdlib>
#include <string>
#include <cmath>
#include <map>

struct Color {//структура типа данных "цвет"
    double r, g, b, a;

    Color(double r, double g, double b, double a) : r(r), g(g), b(b), a(a) {}
};

class Canvas : public Gtk::DrawingArea {
private:
    int state;
    Color color;
    Cairo::RefPtr <Cairo::Surface> buffer;
    Cairo::RefPtr <Cairo::Surface> temp_buffer;
    double buffer_width, buffer_height;
    double start_x, start_y;
    bool need_fix_temp_buffer;

    Gtk::ColorChooserDialog *color_chooser_dialog;
    Gtk::Label *printed_graph_label;//распечатка графа

    std::map<char, std::pair<int, int>> coords;//координаты вершин
public:
    static const int DEFAULT = 1 << 0;//состояния программы
    static const int DRAWING = 1 << 1;
    static const int VERTEX = 1 << 2;
    static const int EDGE = 1 << 3;

    Canvas(Gtk::Label *printed_graph_label);

    void choose_color_response(int response_id);

    void choose_color();

    bool on_mouse_press(GdkEventButton *event);

    bool on_mouse_move(GdkEventMotion *event);

    bool on_mouse_release(GdkEventButton *event);

    Cairo::RefPtr <Cairo::Context> get_context(Cairo::RefPtr <Cairo::Surface> &surface, bool need_clear = false);

    void drawing_vertex(double x, double y, char name);

    void drawing(double x, double y);

    bool on_draw(const Cairo::RefPtr <Cairo::Context> &cr) override;

    void change_tool(int tool);

    static std::string graph_output();

    void print_graph(Gtk::Button *btn);

};


#endif //UNTITLED_CANVAS_H

//
// Created by Dmitriy on 02.04.2024.
//

#include "Canvas.h"
#include "gtkmm.h"
#include "cairomm/context.h"
#include <cstdlib>
#include <string>
#include <cmath>
#include <map>

std::map<char, std::vector<char>> adjacent;//словарь смежности

std::string TITLES = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";//названия вершин
int ID_NEXT_TITLE = 0;//номер следующей вершины для выбора

Canvas::Canvas(Gtk::Label *printed_graph_label) : state(DEFAULT | VERTEX), color(0, 0, 0, 1), buffer_width(1920),
                                                  buffer_height(1080),
                                                  need_fix_temp_buffer(false) {
    //конструктор. В него пришлось передавать лейбл распечатки, потому что по-другому не получилось.
    // Остальные параметры понятны по названию
    this->signal_draw().connect(sigc::mem_fun(*this, &Canvas::on_draw));
    this->set_vexpand(true);
    this->add_events(Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
    this->signal_button_press_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_press));
    this->signal_motion_notify_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_move));
    this->signal_button_release_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_release));
    this->buffer = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, buffer_width, buffer_width);
    this->temp_buffer = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, buffer_width, buffer_width);
    auto context = Cairo::Context::create(this->buffer);//рисование холста (заливка белым)
    context->set_source_rgb(1, 1, 1);
    context->rectangle(0, 0, buffer_width, buffer_height);
    context->fill();
    context->stroke();

    this->color_chooser_dialog = new Gtk::ColorChooserDialog;//настройка диалога выбора цвета
    this->color_chooser_dialog->set_modal(true);
    this->color_chooser_dialog->signal_response().connect(sigc::mem_fun(*this, &Canvas::choose_color_response));

    this->printed_graph_label = printed_graph_label;
}


void Canvas::choose_color_response(int response_id) {//настройка цвета, выбранного через диалог
    if (response_id == Gtk::RESPONSE_OK) {
        auto res = this->color_chooser_dialog->get_rgba();
        this->color = {res.get_red(), res.get_green(), res.get_blue(), res.get_alpha()};

    }
    this->color_chooser_dialog->hide();
}

void Canvas::choose_color() {//запуск диалога выбора цвета
    this->color_chooser_dialog->run();
}

bool Canvas::on_mouse_press(GdkEventButton *event) {//прописывание функционала при нажатой мыши
    this->state &= ~DEFAULT;
    this->state |= DRAWING;
    auto context = this->get_context(temp_buffer, true);
    this->start_x = event->x;
    this->start_y = event->y;
    this->drawing(event->x, event->y);
    return true;
}

bool Canvas::on_mouse_move(GdkEventMotion *event) {//прописывание функционала при двигающейся мыши
    this->state &= ~DEFAULT;//замена состояния
    this->state |= DRAWING;
    if ((this->state & VERTEX) ==
        0) {//если мы рисуем вершину, то нарисовав ее в начале, нам не надо дальше рисовать их, пока зажата мышь
        this->drawing(event->x, event->y);
        return true;
    }
    return true;
}

bool Canvas::on_mouse_release(GdkEventButton *event) {//прописывание функционала при отпускании мыши
    this->state &= ~DRAWING;//замена состояния
    this->state |= DEFAULT;
    this->drawing(event->x, event->y);
    return true;
}

Cairo::RefPtr<Cairo::Context> Canvas::get_context(Cairo::RefPtr<Cairo::Surface> &surface, bool need_clear) {
    //функция "вытаскивания" контекста (раньше мы называли его "cr") для рисования
    auto context = Cairo::Context::create(surface);
    if (need_clear) {//очищаем контекст при надобности
        context->set_source_rgba(0, 0, 0, 0);
        context->set_operator(Cairo::OPERATOR_CLEAR);
        context->rectangle(0, 0, buffer_width, buffer_height);
        context->paint_with_alpha(1);
        context->stroke();
    }
    context->set_source_rgba(this->color.r, this->color.g, this->color.b, this->color.a);
    //настраиваем выбранный ранее цвет
    context->set_operator(Cairo::OPERATOR_OVER);
    return context;
}

void Canvas::drawing_vertex(double x, double y, char name) {
    //функция рисования вершин (с белой подложкой для "перекрытия" наложенных на вершину рёбер)
    std::string s;//перевод названия в тип string для "CLang-Tidy, как просит Clion)
    s.push_back(name);

    auto context = this->get_context(temp_buffer);

    context->set_source_rgba(1, 1, 1, 1);//рисование подложки
    context->arc(x, y, 20, 0, M_PI * 2);
    context->fill();

    context = this->get_context(temp_buffer);

    context->arc(x, y, 20, 0, M_PI * 2);
    context->set_line_width(6);
    context->stroke();

    cairo_text_extents_t te;//рисование названия вершины
    context->select_font_face("Calibri", static_cast<Cairo::FontSlant>(CAIRO_FONT_SLANT_NORMAL),
                              static_cast<Cairo::FontWeight>(CAIRO_FONT_WEIGHT_BOLD));
    context->set_font_size(20);
    context->get_text_extents(s, te);
    context->move_to(x - 6, y + 5);
    context->show_text(s);

    context->stroke();
}

void Canvas::drawing(double x, double y) {//функция состояния рисования
    if (this->state & DRAWING) {
        this->need_fix_temp_buffer = true;
        if (this->state & VERTEX) {//рисование вершины
            bool can_draw = true;
            for (auto i: this->coords) {//проверка на дальность от других вершин для защиты от наложения
                if (abs(i.second.first - x) <= 80 and abs(i.second.second - y) <= 80) {
                    can_draw = false;
                    break;
                }
            }
            if (can_draw and
                ID_NEXT_TITLE != TITLES.size()) {//+проверка на максимальное кол-во вершин, иначе не рисуем
                char next_char = TITLES[ID_NEXT_TITLE];
                ID_NEXT_TITLE++;
                this->coords[next_char] = {x, y};//добавляем новую вершину туда, куда нужно
                adjacent[next_char] = {};

                drawing_vertex(x, y, next_char);//отрисовываем вершину
            }
        }
        if (this->state & EDGE) {//рисование ребра
            auto context = this->get_context(temp_buffer, true);
            context->set_line_width(2);

            char sticking_from_vertex = '-';//"прилипание" к начальной вершине ребра
            for (auto i: this->coords) {
                if (abs(i.second.first - start_x) <= 40 and abs(i.second.second - start_y) <= 40) {
                    start_x = i.second.first;
                    start_y = i.second.second;
                    sticking_from_vertex = i.first;
                    break;
                }
            }
            if (sticking_from_vertex !=
                '-') {//если ребро рисуется не из воздуха, а из существующей вершины, то продолжаем
                char sticking_to_vertex = '-';//проверка на "прилипание" к конечной вершине ребра

                for (auto i: this->coords) {
                    if (abs(i.second.first - x) <= 40 and abs(i.second.second - y) <= 40) {
                        x = i.second.first;
                        y = i.second.second;
                        sticking_to_vertex = i.first;
                        break;
                    }
                }
                if (sticking_to_vertex != sticking_from_vertex and sticking_to_vertex != '-') {
                    //если начальная вершина не совпадает с конечной и конечная не в воздухе, то рисуем
                    bool edge_exist = false;//проверка на существование ребра
                    for (auto i: adjacent[sticking_from_vertex]) {
                        if (sticking_to_vertex == i) {
                            edge_exist = true;
                            break;
                        }
                    }
                    if (!edge_exist) {//если ребро не существует, добавляем его в словарь смежности
                        adjacent[sticking_from_vertex].push_back(sticking_to_vertex);
                    }
                    context->move_to(this->start_x, this->start_y);//рисование самой линии
                    context->line_to(x, y);
                    context->stroke();

                    drawing_vertex(this->start_x, this->start_y, sticking_from_vertex);
                    //повторние рисование вершин, чтобы перекрыть края линии, заехавшие на вершину
                    drawing_vertex(x, y, sticking_to_vertex);

                    double a = atan(((x - start_x) * 1.0) / (y - start_y));//расчёт угла наклона вектора линии
                    a = 90 - a * 180 / M_PI;
                    if (y - start_y < 0) {
                        a += 180;
                    }

                    x = (x + start_x) / 2;//переход к середине отрезка
                    y = (y + start_y) / 2;
                    context->move_to(x, y);
                    int length_arrow_straight = 10;//длина прямой палочки стрелки
                    int length_arrow_inclined = round(
                            (double) length_arrow_straight / sqrt(2));//длина наклонной палочки стрелки

                    //все ифы определяют, в какую сторону будут направлены палочки у стрелки в зависимости от угла наклона линии
                    if (a >= 0 and a < 15 or a >= 345 and a < 360) {
                        context->line_to(x - length_arrow_inclined, y + length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_inclined, y - length_arrow_inclined);
                    } else if (a >= 15 and a < 30) {
                        context->line_to(x - length_arrow_straight, y);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_inclined, y - length_arrow_inclined);
                    } else if (a >= 30 and a < 60) {
                        context->line_to(x - length_arrow_straight, y);
                        context->move_to(x, y);
                        context->line_to(x, y - length_arrow_straight);
                    } else if (a >= 60 and a < 75) {
                        context->line_to(x - length_arrow_inclined, y - length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x, y - length_arrow_straight);
                    } else if (a >= 75 and a < 105) {
                        context->line_to(x - length_arrow_inclined, y - length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_inclined, y - length_arrow_inclined);
                    } else if (a >= 105 and a < 120) {
                        context->line_to(x, y - length_arrow_straight);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_inclined, y - length_arrow_inclined);
                    } else if (a >= 120 and a < 150) {
                        context->line_to(x, y - length_arrow_straight);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_straight, y);
                    } else if (a >= 150 and a < 165) {
                        context->line_to(x + length_arrow_inclined, y - length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_straight, y);
                    } else if (a >= 165 and a < 195) {
                        context->line_to(x + length_arrow_inclined, y - length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_inclined, y + length_arrow_inclined);
                    } else if (a >= 195 and a < 210) {
                        context->line_to(x + length_arrow_straight, y);
                        context->move_to(x, y);
                        context->line_to(x + length_arrow_inclined, y + length_arrow_inclined);
                    } else if (a >= 210 and a < 240) {
                        context->line_to(x + length_arrow_straight, y);
                        context->move_to(x, y);
                        context->line_to(x, y + length_arrow_straight);
                    } else if (a >= 240 and a < 255) {
                        context->line_to(x + length_arrow_inclined, y + length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x, y + length_arrow_straight);
                    } else if (a >= 255 and a < 285) {
                        context->line_to(x + length_arrow_inclined, y + length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_inclined, y + length_arrow_inclined);
                    } else if (a >= 285 and a < 300) {
                        context->line_to(x, y + length_arrow_straight);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_inclined, y + length_arrow_inclined);
                    } else if (a >= 300 and a < 330) {
                        context->line_to(x, y + length_arrow_straight);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_straight, y);
                    } else if (a >= 330 and a < 345) {
                        context->line_to(x - length_arrow_inclined, y + length_arrow_inclined);
                        context->move_to(x, y);
                        context->line_to(x - length_arrow_straight, y);
                    }

                    context->stroke();
                }
            }

        }
        this->queue_draw();
    } else if (this->need_fix_temp_buffer) {
        this->need_fix_temp_buffer = false;
        auto context = this->get_context(buffer);
        context->set_source(this->temp_buffer, 0, 0);
        context->paint();
        this->queue_draw();
    }
}

bool Canvas::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->set_source(this->buffer, 0, 0);
    cr->paint();
    if (this->state & DRAWING) {
        cr->set_source(this->temp_buffer, 0, 0);
        cr->paint();
    }
    return true;
}

void Canvas::change_tool(int tool) {//функция смены инструмента
    this->state = Canvas::DEFAULT | tool;
    if (tool == VERTEX) {

    }
}

std::string Canvas::graph_output() {//функция формирования словаря смежности для вывода
    std::string output;
    int count_vertexes = (int) adjacent.size();
    for (auto i: adjacent) {
        char vertex = i.first;
        output.push_back(vertex);
        output += ":";
        for (auto edge: i.second) {
            output += " ";
            output.push_back(edge);
        }
        if (i.second.empty()) {
            output += " None";
        }
        output += ";\n";
    }
    return output;
}

void Canvas::print_graph(Gtk::Button *btn) {//функция распечатывания графа
    if (btn->get_label() == "Print Graph") {//"развёртывание" лейбла с распечаткой
        this->printed_graph_label->set_text(graph_output());
        this->printed_graph_label->show();
        btn->set_label("Close printout");
    } else {//"свёртывание" лейбла с распечаткой
        this->printed_graph_label->hide();
        btn->set_label("Print Graph");
    }
}

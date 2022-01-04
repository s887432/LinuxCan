/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <random>
#include <chrono>
#include <queue>
#include <poll.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <thread>

//#define SHOW_SPEED
//#define SHOW_GEAR
//#define SHOW_TEMP
//#define SHOW_FUEL
//#define SHOW_ENG5TC
//#define SHOW_PHONECALL
//#define SHOW_BAR
//#define SHOW_VSC
//#define SHOW_ENG
//#define SHOW_WIFI
//#define SHOW_BLINK

#define SHOW_NEEDLE
#define SHOW_HEADLIGHT
#define SHOW_TURNLIGHT

#define HAVE_LIBPLANES

typedef enum __CAN_DEVICE__ {
  CAN_NONE = 0,
  CAN_NEEDLE,
  CAN_SPEED,
  CAN_GEAR,
  CAN_TEMP,
  CAN_FUEL,
  CAN_ENG5TC,
  CAN_PHONECALL,
  CAN_BAR,
  CAN_VSC,
  CAN_WIFI,
  CAN_BLINK
} CAN_DEVICE;

#ifdef HAVE_LIBPLANES
#include <planes/plane.h>
#include "egt/detail/screen/kmsoverlay.h"
#endif
#include "../src/detail/erawimage.h"
#include "../src/detail/eraw.h"


#define DO_SVG_SERIALIZATION

#define MEDIA_FILE_PATH "/root/"
#define solve_relative_path(relative_path) (MEDIA_FILE_PATH + relative_path)

#ifdef HAVE_LIBPLANES
#define SCALE_TO_MAX_AND_STAY    //If this macro defined, scale animation will scale from min to max and stay, then enter to demo.
                                 //Else if not defined, scale animation will scale from min to max, and then scale to min, after that enter to demo.
#endif

#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms

#define ID_MAX               1327

#define ID_MIN               847
#define STEPPER              4
#define ID_MAX_FUEL          960
#define ID_MIN_FUEL          946
#define FUEL_STEPPER         2
#define ID_MAX_TEMP          983
#define ID_MIN_TEMP          971
#define ID_MAX_TEXT_LEFT     1027
#define ID_MIN_TEXT_LEFT     995
#define ID_MAX_TEXT_RIGHT    1067
#define ID_MIN_TEXT_RIGHT    1031
#define ID_MAX_TEXT_GEAR     1383  //STEPPER=4
#define ID_MIN_TEXT_GEAR     1367
#define ID_MAX_LEFT_SPEED_L  1341  //STEPPER=4
#define ID_MIN_LEFT_SPEED_L  1309
#define ID_MAX_LEFT_SPEED_R  1381  //STEPPER=4
#define ID_MIN_LEFT_SPEED_R  1345
#define ID_MAX_RIGHT_SPEED_L 1417  //STEPPER=4
#define ID_MIN_RIGHT_SPEED_L 1385
#define ID_MAX_RIGHT_SPEED_R 1457  //STEPPER=4
#define ID_MIN_RIGHT_SPEED_R 1421
#define ID_MAX_TEMP_L        1499  //STEPPER=4
#define ID_MIN_TEMP_L        1487
#define ID_MAX_TEMP_R        1539  //STEPPER=4
#define ID_MIN_TEMP_R        1503
#define MAX_TEMP_TABLE       12
#define MAX_SPEED_TABLE      9
#define MAX_DIGIT_PAIR       2
#define MAX_FUEL_REC         8
#define MAX_TEMP_REC         7
#define PLANE_WIDTH_HF       400
#define PLANE_HEIGHT_HF      240
#define MAX_NEEDLE_INDEX     120

int g_digit = ID_MIN;
bool is_increasing = true;
int g_fuel_digit = ID_MIN_FUEL;
bool is_fuel_inc = true;
int g_temp_digit = ID_MAX_TEMP;
bool is_temp_inc = true;
int g_left_txt_digit = ID_MIN_TEXT_LEFT;
int g_right_txt_digit = ID_MIN_TEXT_RIGHT;
bool is_txt_inc = true;
int g_gear_txt_digit = ID_MIN_TEXT_GEAR;
bool is_gear_inc = true;
int g_left_txt_temp = ID_MIN_TEMP_L;
bool is_left_temp_inc = true;
int g_right_txt_temp = ID_MIN_TEMP_R;
bool is_right_temp_inc = true;


enum class SvgEleType
{
    path = 0,
    text,
    bar,
    rbar
};

typedef struct
{
    int main_speed[MAX_DIGIT_PAIR];
    int left_speed[MAX_DIGIT_PAIR];
    int right_speed[MAX_DIGIT_PAIR];
    int speed_digit;
}st_speed_table_t;

typedef struct
{
    int pan_x;
    int pan_y;
    int pan_w;
    int pan_h;

    int x;
    int y;
}st_plane_attri_t;

typedef struct
{
    int y;
    int h;
}st_fuel_temp_t;

st_plane_attri_t g_fuel[MAX_FUEL_REC] = {
    /* panx, pany, panw, panh, x, y */
    {  0, 0, 23,  22, 17, 328},  //Y bottom=350
    { 23, 0, 23,  44, 17, 306},
    { 46, 0, 23,  66, 17, 284},
    { 69, 0, 23,  88, 17, 262},
    { 92, 0, 23, 110, 17, 240},
    {115, 0, 23, 132, 17, 218},
    {138, 0, 23, 154, 17, 196},
    {161, 0, 23, 175, 17, 175}
};

st_plane_attri_t g_temp[MAX_TEMP_REC] = {
    /* panx, pany, panw, panh, x, y */
    {  0, 0, 23,  24, 757, 324},  //Y bottom=348
    { 23, 0, 23,  51, 757, 297},
    { 46, 0, 23,  77, 757, 271},
    { 69, 0, 23, 103, 757, 245},
    { 92, 0, 23, 129, 757, 219},
    {115, 0, 23, 156, 757, 192},
    {138, 0, 23, 181, 757, 167}
};

st_fuel_temp_t g_fuel_table[7] = {
    {173, 176},
    {194, 155},
    {216, 133},
    {238, 111},
    {259, 90},
    {281, 68},
    {303, 46}
};

st_fuel_temp_t g_temp_table[6] = {
    {324, 25},
    {298, 51},
    {271, 78},
    {245, 104},
    {220, 129},
    {194, 167}
};


class MotorDash : public egt::experimental::Gauge
{
public:

#ifdef DO_SVG_SERIALIZATION
    MotorDash(egt::SvgImage& svg) noexcept: m_svg(svg)
#else
    MotorDash() noexcept
#endif
    {
        set_gear_deserial_state(false);
        set_fuel_deserial_state(false);
        set_temp_deserial_state(false);
        set_needle_deserial_state(false);
        set_speed_deserial_state(false);
        set_call_deserial_state(false);
        set_bar_deserial_state(false);
        set_blink_deserial_state(false);
        set_tc_deserial_state(false);
        set_vsc_deserial_state(false);
        set_bat_deserial_state(false);
        set_snow_deserial_state(false);
        set_high_q_state(true);
        set_low_q_state(true);
        set_wifi_deserial_state(false);
		
		set_headlight_deserial_state(false);
		set_turnlight_deserial_state(false);
    }

#ifdef DO_SVG_SERIALIZATION
    void serialize_all();

    void render_needles()
    {
        for (int i = ID_MIN; i <= ID_MAX; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#path", i, false);
        }
    }

    void render_needles(int render_start, int render_end)
    {
        for (int i = render_start; i <= render_end; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#path", i, false);
        }
    }

    void render_needles(int render_digit)
    {
        add_svg_layer_with_digit(m_svg, "#path", render_digit, true);
    }

    void render_left_spd()
    {
        for (int i = ID_MIN_TEXT_LEFT; i <= ID_MAX_TEXT_LEFT; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
        //std::cout << "render_left_spd" << std::endl;
    }

    void render_right_spd()
    {
        for (int i = ID_MIN_TEXT_RIGHT; i <= ID_MAX_TEXT_RIGHT; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
        //std::cout << "render_right_spd" << std::endl;
    }

    void render_gear()
    {
        //GEAR UI
        for (int i = ID_MIN_TEXT_GEAR; i <= ID_MAX_TEXT_GEAR; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_bar()
    {
        add_svg_layer(m_svg, "#bar0", true);
        add_svg_layer(m_svg, "#rbar0", true);
    }

    void render_bar5()
    {
        add_svg_layer(m_svg, "#bar5", true);
        add_svg_layer(m_svg, "#rbar5", true);
    }

    void render_bar8()
    {
        add_svg_layer(m_svg, "#bar8", true);
        add_svg_layer(m_svg, "#rbar8", true);
    }

    void render_l_spd_l()
    {
        //left speed ID_MIN_LEFT_SPEED_L
        for (int i = ID_MIN_LEFT_SPEED_L; i <= ID_MAX_LEFT_SPEED_L; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_l_spd_r()
    {
        //left speed ID_MIN_LEFT_SPEED_R
        for (int i = ID_MIN_LEFT_SPEED_R; i <= ID_MAX_LEFT_SPEED_R; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_r_spd_l()
    {
        //left speed ID_MIN_RIGHT_SPEED_L
        for (int i = ID_MIN_RIGHT_SPEED_L; i <= ID_MAX_RIGHT_SPEED_L; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_r_spd_r()
    {
        //left speed ID_MIN_RIGHT_SPEED_R
        for (int i = ID_MIN_RIGHT_SPEED_R; i <= ID_MAX_RIGHT_SPEED_R; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_temp_l()
    {
        //left temperature ID_MIN_TEMP_L
        for (int i = ID_MIN_TEMP_L; i <= ID_MAX_TEMP_L; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }

    void render_temp_r()
    {
        //right temperature ID_MIN_TEMP_R
        for (int i = ID_MIN_TEMP_R; i <= ID_MAX_TEMP_R; i += STEPPER)
        {
            add_svg_layer_with_digit(m_svg, "#text", i, true);
        }
    }
#endif

    void test()
    {
        auto text = std::make_shared<egt::TextBox>("5", egt::Rect(egt::Point(82, 1), egt::Size(20, 2)));
        text->border(0);
        text->font(egt::Font(40, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, egt::Palette::white);
        add(text);
    }

    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size)
    {
        auto text = std::make_shared<egt::TextBox>(txt, rect, egt::AlignFlag::center);
        text->border(0);
        text->font(egt::Font(size, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, egt::Palette::white);
        add(text);
        text->name(id);
    }

    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size, egt::Color color)
    {
        auto text = std::make_shared<egt::TextBox>(txt, rect);
        text->border(0);
        text->font(egt::Font(size, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, color);
        add(text);
        text->name(id);
    }

#ifdef DO_SVG_SERIALIZATION
    void add_rec_widget(const std::string& id, bool is_hiding)
    {
        if (!m_svg.id_exists(id))
            return;

        auto box = m_svg.id_box(id);
        auto rec = std::make_shared<egt::RectangleWidget>(egt::Rect(std::floor(box.x()),
                                                                    std::floor(box.y()),
                                                                    std::ceil(box.width()),
                                                                    std::ceil(box.height())));
        rec->color(egt::Palette::ColorId::button_bg, egt::Palette::black);
        if (is_hiding)
            rec->hide();
        add(rec);
        rec->name(id);
    }
#endif

    void add_rec_widget_with_rec(const std::string& id, const egt::Rect& rect, bool is_hiding)
    {
        auto rec = std::make_shared<egt::RectangleWidget>(rect);
        rec->color(egt::Palette::ColorId::button_bg, egt::Palette::black);
        if (is_hiding)
            rec->hide();
        add(rec);
        rec->name(id);
    }

    std::shared_ptr<egt::TextBox> find_text(const std::string& name)
    {
        return find_child<egt::TextBox>(name);
    }

    std::shared_ptr<egt::RectangleWidget> find_rec(const std::string& name)
    {
        return find_child<egt::RectangleWidget>(name);
    }

    std::shared_ptr<egt::experimental::GaugeLayer> find_layer(const std::string& name)
    {
        return find_child<egt::experimental::GaugeLayer>(name);
    }

    void set_text(const std::string& id, const std::string& text, int font_size, const egt::Pattern& color)
    {
        auto layer = find_rec(id);
        auto ptext = std::make_shared<egt::Label>();
        ptext->text_align(egt::AlignFlag::center);
        ptext->box(egt::Rect(layer->box().x(), layer->box().y(), layer->box().width(), layer->box().height()));
        ptext->color(egt::Palette::ColorId::label_text, color);
        ptext->font(egt::Font(font_size, egt::Font::Weight::normal));
        ptext->text(text);
        add(ptext);
    }

    void set_color(const egt::Color& color)
    {
        for (auto i = ID_MIN; i <= ID_MAX; i += STEPPER)
        {
            std::ostringstream ss;
            ss << "#path" << std::to_string(i);
            find_layer(ss.str())->mask_color(color);
        }
    }

    void apply(int digit, bool is_inc, SvgEleType type)
    {
        //std::cout << "apply: " << digit << "  type:" << static_cast<int>(type) << "  inc:" << is_inc << std::endl;
        std::ostringstream ss;
        switch (type)
        {
            case SvgEleType::path:
                ss << "#path" << std::to_string(digit);
                break;
            case SvgEleType::text:
                ss << "#text" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            case SvgEleType::bar:
                ss << "#bar" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            case SvgEleType::rbar:
                ss << "#rbar" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            default:
                break;
        }

        if (is_inc)
        {
            //std::cout << "show: " << ss.str() << std::endl;
            find_layer(ss.str())->show();
        }
        else
        {
            //std::cout << "hide: " << ss.str() << std::endl;
            find_layer(ss.str())->hide();
        }
    }

    void show_digit(int digit)
    {
        std::ostringstream ss;
        ss << "#path" << std::to_string(digit);
        //std::cout << "show_digit: " << ss.str() << std::endl;
        find_layer(ss.str())->show();
    }

    void hide_some(int start, int loop)
    {
        for (int i=0; i < loop; i++)
        {
            std::ostringstream ss;
            ss << "#path" << std::to_string(start);
            //std::cout << "hide_some: " << ss.str() << std::endl;
            find_layer(ss.str())->hide();
            start += STEPPER;
        }
    }

    void hide_speed_text()
    {
        for (int i=ID_MIN_TEXT_LEFT; i <= ID_MAX_TEXT_RIGHT; i += STEPPER)
        {
            std::ostringstream ss;
            ss << "#text" << std::to_string(i);
            find_layer(ss.str())->hide();
        }
    }

    void hide_left_speed_text()
    {
        for (int i=ID_MIN_LEFT_SPEED_L; i <= ID_MAX_LEFT_SPEED_R; i += STEPPER)
        {
            std::ostringstream ss;
            ss << "#text" << std::to_string(i);
            find_layer(ss.str())->hide();
        }
    }

    void hide_right_speed_text()
    {
        for (int i=ID_MIN_RIGHT_SPEED_L; i <= ID_MAX_RIGHT_SPEED_R; i += STEPPER)
        {
            std::ostringstream ss;
            ss << "#text" << std::to_string(i);
            find_layer(ss.str())->hide();
        }
    }

    void hide_all_needles(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> NeedleBase)
    {
        for (int i = 0; i <= MAX_NEEDLE_INDEX; i++)
        {
            NeedleBase[i]->hide();
        }
    }

    void hide_gear_text(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> GearBase)
    {
        for (int i = 0; i <= 4; i++)
        {
            GearBase[i]->hide();
        }
    }

    void hide_fuel_rect(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> FuelBase)
    {
        for (int i = 0; i <= 7; i++)
        {
            FuelBase[i]->hide();
        }
    }

    void hide_temp_rect(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> TempBase)
    {
        for (int i = 0; i <= 6; i++)
        {
            TempBase[i]->hide();
        }
    }

    void hide_bar_text()
    {
        find_layer("#bar5")->hide();
        find_layer("#rbar5")->hide();
        find_layer("#bar8")->hide();
        find_layer("#rbar8")->hide();
    }

    void hide_all()
    {
        for (auto& child : m_children)
        {
            if (child->name().rfind("#path", 0) == 0)
                child->hide();
        }
    }

    bool get_high_q_state() { return m_is_high_q_quit; }
    void set_high_q_state(bool is_high_q_quit) { m_is_high_q_quit = is_high_q_quit; }

    bool get_low_q_state() { return m_is_low_q_quit; }
    void set_low_q_state(bool is_low_q_quit) { m_is_low_q_quit = is_low_q_quit; }

    bool get_gear_deserial_state() { return m_gear_deserial_finish; }
    void set_gear_deserial_state(bool gear_deserial_finish) { m_gear_deserial_finish = gear_deserial_finish; }

    bool get_fuel_deserial_state() { return m_fuel_deserial_finish; }
    void set_fuel_deserial_state(bool fuel_deserial_finish) { m_fuel_deserial_finish = fuel_deserial_finish; }

    bool get_temp_deserial_state() { return m_temp_deserial_finish; }
    void set_temp_deserial_state(bool temp_deserial_finish) { m_temp_deserial_finish = temp_deserial_finish; }

    bool get_needle_deserial_state() { return m_needle_deserial_finish; }
    void set_needle_deserial_state(bool needle_deserial_finish) { m_needle_deserial_finish = needle_deserial_finish; }

    bool get_speed_deserial_state() { return m_speed_deserial_finish; }
    void set_speed_deserial_state(bool speed_deserial_finish) { m_speed_deserial_finish = speed_deserial_finish; }

    bool get_call_deserial_state() { return m_call_deserial_finish; }
    void set_call_deserial_state(bool call_deserial_finish) { m_call_deserial_finish = call_deserial_finish; }

    bool get_bar_deserial_state() { return m_bar_deserial_finish; }
    void set_bar_deserial_state(bool bar_deserial_finish) { m_bar_deserial_finish = bar_deserial_finish; }

    bool get_blink_deserial_state() { return m_blink_deserial_finish; }
    void set_blink_deserial_state(bool blink_deserial_finish) { m_blink_deserial_finish = blink_deserial_finish; }

    bool get_tc_deserial_state() { return m_tc_deserial_finish; }
    void set_tc_deserial_state(bool tc_deserial_finish) { m_tc_deserial_finish = tc_deserial_finish; }

    bool get_vsc_deserial_state() { return m_vsc_deserial_finish; }
    void set_vsc_deserial_state(bool vsc_deserial_finish) { m_vsc_deserial_finish = vsc_deserial_finish; }


	bool get_headlight_deserial_state() { return m_headlight_deserial_finish; }
    void set_headlight_deserial_state(bool headlight_deserial_finish) { m_headlight_deserial_finish = headlight_deserial_finish; }
	bool get_turnlight_deserial_state() { return m_turnlight_deserial_finish; }
    void set_turnlight_deserial_state(bool turnlight_deserial_finish) { m_turnlight_deserial_finish = turnlight_deserial_finish; }

    bool get_bat_deserial_state() { return m_bat_deserial_finish; }
    void set_bat_deserial_state(bool bat_deserial_finish) { m_bat_deserial_finish = bat_deserial_finish; }

    bool get_snow_deserial_state() { return m_snow_deserial_finish; }
    void set_snow_deserial_state(bool snow_deserial_finish) { m_snow_deserial_finish = snow_deserial_finish; }

    bool get_wifi_deserial_state() { return m_wifi_deserial_finish; }
    void set_wifi_deserial_state(bool wifi_deserial_finish) { m_wifi_deserial_finish = wifi_deserial_finish; }

    //egt::Application *app_ptr = nullptr;
    int needle_digit = ID_MIN;

    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    void ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect);

#ifdef DO_SVG_SERIALIZATION
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id);
    void SerializePNG(const char* png_src, const std::string& png_dst);
#endif

private:
#ifdef DO_SVG_SERIALIZATION
    void add_svg_layer(egt::SvgImage& svg, const std::string& ss, bool is_hiding)
    {
        if (nullptr != find_layer(ss))
            return;
        auto layer = std::make_shared<egt::experimental::GaugeLayer>(svg.render(ss));
        if (is_hiding)
            layer->hide();
        add(layer);
        layer->name(ss);
        //std::cout << "render widget:" << ss << std::endl;
    }

    void add_svg_layer_with_digit(egt::SvgImage& svg, const std::string& ss, int digit, bool is_hiding)
    {
        std::ostringstream str;
        str << ss << std::to_string(digit);
        if (nullptr != find_layer(str.str()))
            return;
        auto box = svg.id_box(str.str());
        auto layer = std::make_shared<egt::experimental::GaugeLayer>(svg.render(str.str(), box));

        layer->name(str.str());
        layer->box(egt::Rect(box.x() + moat(),
                                box.y() + moat(),
                                std::ceil(box.width()),
                                std::ceil(box.height())));
        if (is_hiding)
            layer->hide();
        add(layer);
        //app_ptr->event().step();
        needle_digit += STEPPER;
        //std::cout << "render:" << str.str() << std::endl;
    }
    egt::SvgImage& m_svg;
#endif

    bool m_is_high_q_quit;
    bool m_is_low_q_quit;
    bool m_gear_deserial_finish;
    bool m_fuel_deserial_finish;
    bool m_temp_deserial_finish;
    bool m_needle_deserial_finish;
    bool m_speed_deserial_finish;
    bool m_call_deserial_finish;
    bool m_bar_deserial_finish;
    bool m_blink_deserial_finish;
    bool m_tc_deserial_finish;
    bool m_vsc_deserial_finish;
    bool m_bat_deserial_finish;
    bool m_snow_deserial_finish;
    bool m_wifi_deserial_finish;

	bool m_headlight_deserial_finish;
	bool m_turnlight_deserial_finish;
};


class DeserializeDash
{
public:
    DeserializeDash() noexcept {}
    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    ~DeserializeDash() {}
};

#ifdef DO_SVG_SERIALIZATION
void MotorDash::SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id)
{
    auto box = svg.id_box(id);
    auto layer = std::make_shared<egt::Image>(svg.render(id, box));

    egt::detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    e.save(solve_relative_path(filename), data, box.x(), box.y(), width, height);
}

void MotorDash::SerializePNG(const char* png_src, const std::string& png_dst)
{
    egt::shared_cairo_surface_t surface;
    egt::detail::ErawImage e;
    surface =
            egt::shared_cairo_surface_t(cairo_image_surface_create_from_png(png_src),
                                        cairo_surface_destroy);
    const auto data = cairo_image_surface_get_data(surface.get());
    const auto width = cairo_image_surface_get_width(surface.get());
    const auto height = cairo_image_surface_get_height(surface.get());
    e.save(solve_relative_path(png_dst), data, 0, 0, width, height);
}

void MotorDash::serialize_all()
{
    int i;
    std::ostringstream str;
    std::ostringstream path;

    //Serialize scale PNG image
    SerializePNG("/usr/share/egt/examples/motorcycledash/moto.png", "eraw/moto_png.eraw");

    //Serialize background image
    SerializeSVG("eraw/bkgrd.eraw", m_svg, "#bkgrd");

    //Serialize Gear
    for (i = ID_MIN_TEXT_GEAR; i <= ID_MAX_TEXT_GEAR; i += STEPPER)
    {
        str.str("");
        path.str("");
        str << "#text" << std::to_string(i);
        path << "eraw/text" << std::to_string(i) << ".eraw";
        SerializeSVG(path.str(), m_svg, str.str());
    }

    //Serialize Needles
    for (i = ID_MIN; i <= ID_MAX; i += STEPPER)
    {
        str.str("");
        path.str("");
        str << "#path" << std::to_string(i);
        path << "eraw/path" << std::to_string(i) << ".eraw";
        SerializeSVG(path.str(), m_svg, str.str());
    }

    //Serialize eng5
    //SerializeSVG("eraw/eng5.eraw", m_svg, "#eng5");

    //SerializeSVG("eraw/fuelrect.eraw", m_svg, "#fuelrect");

    //SerializeSVG("eraw/temprect.eraw", m_svg, "#temprect");

    //SerializeSVG("eraw/fuelr.eraw", m_svg, "#fuelr");

    //SerializeSVG("eraw/tempw.eraw", m_svg, "#tempw");

    //Serialize tc
    //SerializeSVG("eraw/tc.eraw", m_svg, "#tc");

    //Serialize mute
    //SerializeSVG("eraw/mute.eraw", m_svg, "#mute");

    //Serialize takeoff
    //SerializeSVG("eraw/takeoff.eraw", m_svg, "#takeoff");

    //Serialize left_blink
    SerializeSVG("eraw/left_blink.eraw", m_svg, "#left_blink");

    //Serialize right_blink
    SerializeSVG("eraw/right_blink.eraw", m_svg, "#right_blink");

    //Serialize farlight
    SerializeSVG("eraw/farlight.eraw", m_svg, "#farlight");

    //Serialize vsc
    //SerializeSVG("eraw/vsc.eraw", m_svg, "#vsc");

    //Serialize wifi
    //SerializeSVG("eraw/wifi.eraw", m_svg, "#wifi");

    //Serialize bt
    //SerializeSVG("eraw/bt.eraw", m_svg, "#bt");

    //Serialize engine
    //SerializeSVG("eraw/engine.eraw", m_svg, "#engine");

    //Serialize temp
    //SerializeSVG("eraw/temp.eraw", m_svg, "#temp");

    //Serialize calling
    //SerializeSVG("eraw/calling.eraw", m_svg, "#calling");

    //Serialize callname
    //SerializeSVG("eraw/callname.eraw", m_svg, "#callname");

    //Serialize callnum
    //SerializeSVG("eraw/callnum.eraw", m_svg, "#callnum");

    //Serialize mainspeed
    SerializeSVG("eraw/mainspeed.eraw", m_svg, "#mainspeed");

    //Serialize bat
    //SerializeSVG("eraw/bat.eraw", m_svg, "#bat");

    //Serialize egoil
    //SerializeSVG("eraw/egoil.eraw", m_svg, "#egoil");

    //Serialize hazards
    //SerializeSVG("eraw/hazards.eraw", m_svg, "#hazards");

    //Serialize snow
    //SerializeSVG("eraw/snow.eraw", m_svg, "#snow");

    //Serialize abs
    //SerializeSVG("eraw/abs.eraw", m_svg, "#abs");

    //Serialize lspeed
    //SerializeSVG("eraw/lspeed.eraw", m_svg, "#lspeed");

    //Serialize rspeed
    //SerializeSVG("eraw/rspeed.eraw", m_svg, "#rspeed");

    //Serialize lbar
    //SerializeSVG("eraw/lbar.eraw", m_svg, "#lbar");

    //Serialize rbar
    //SerializeSVG("eraw/rbar.eraw", m_svg, "#rbar");

    //Create a finish indicator
    if (-1 == system("touch /root/serialize_done"))
    {
        std::cout << "touch /root/serialize_done failed, please check permission!!!" << std::endl;
        return;
    }
    if (-1 == system("sync"))
    {
        std::cout << "sync failed, please check permission!!!" << std::endl;
        return;
    }
}
#endif

egt::shared_cairo_surface_t DeserializeDash::DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect)
{
    return egt::detail::ErawImage::load(filename, rect);
}

egt::shared_cairo_surface_t MotorDash::DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect)
{
    return egt::detail::ErawImage::load(solve_relative_path(filename), rect);
}

//When drawing in Inkscape, the y is starting from bottom but not from top,
//so we need convert this coordinate to EGT using, the y is starting from top
void MotorDash::ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect)
{
    rect->y(screen()->size().height() - rect->y() - rect->height());
}

//using QueueCallback = std::function<void ()>;
using QueueCallback = std::function<std::string ()>;

typedef enum __TURN_LIGHT__
{
	TURN_LIGHT_OFF = 0,
	TURN_LIGHT_RIGHT = 0x10,
	TURN_LIGHT_LEFT = 0x01
}TURN_LIGHT;

typedef enum __HEAD_LIGHT__
{
	HEAD_LIGHT_OFF = 0,
	HEAD_LIGHT_ON
}HEAD_LIGHT;

typedef struct __MOTO_STRUCT__
{
    int gear_index;
    int speed_index;
    int needle_index;
    int timer_cnt;
    int lbar;
    int temp_index;
    int fuel_index;

	HEAD_LIGHT head_light;
	TURN_LIGHT turn_light;

	int value_changed;
} MOTO_STRUCT, *pMORO_STRUCT;

int main(int argc, char** argv)
{
    std::cout << "EGT start" << std::endl;
    //int timediff = 0;
    //struct timeval time1, time2;
    //gettimeofday(&time1, NULL);

    std::queue<QueueCallback> high_pri_q;
    std::queue<QueueCallback> low_pri_q;

    //Widget handler
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> NeedleBase;
    //std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> GearBase;
    //std::shared_ptr<egt::experimental::GaugeLayer> TempPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> FuelPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> TempwPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> FuelrPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> callingPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> mutePtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> takeoffPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> left_blinkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> right_blinkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> farlightPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> vscPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> wifiPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> btPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> enginePtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> batPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> egoilPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> hazardsPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> snowPtr;
    //std::shared_ptr<egt::experimental::GaugeLayer> absPtr;

    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    std::ostringstream path;

#ifdef DO_SVG_SERIALIZATION
    bool need_serialization = false;
#endif

#ifdef HAVE_LIBPLANES
    //Scale effect variables
    float scale_factor = 0.01;
    bool is_scale_rev = false;
    bool is_scale_2_max = false;
    bool is_scale_finish = false;
#endif

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;
    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

#ifdef EXAMPLEDATA
    egt::add_search_path(EXAMPLEDATA);
#endif

#ifdef DO_SVG_SERIALIZATION
    //Check if serialize indicator "/serialize_done" exist? If not, need serialization
    if (access("/root/serialize_done", F_OK))
    {
        if (-1 == system("rm -rf /root/eraw"))
        {
            std::cout << "rm -rf /root/eraw failed, please check permission!!!" << std::endl;
            return -1;
        }
        if (0 > mkdir("/root/eraw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            std::cout << "Create serialization dir eraw failed, please check permission!!!" << std::endl;
            return -1;
        }
        else
            need_serialization = true;
    }

    //If need serialization, use serialize.svg to parse SVG parameters, else use a blank one
    std::string svgpath = need_serialization ? "file:serialize.svg" : "file:deserialize.svg";
    auto svg = std::make_unique<egt::SvgImage>(svgpath, egt::SizeF(window.content_area().width(), 0));
    MotorDash motordash(*svg);
#else
    MotorDash motordash;
#endif

    window.add(motordash);
    motordash.show();
    window.show();
    motordash.add_text_widget("#textinit", "0", egt::Rect(900, 600, 2, 2), 1);
    std::cout << "EGT show" << std::endl;

    //Lambda for de-serializing background and needles
    auto DeserialNeedles = [&]()
    {
        //Background image and needles should be de-serialized firstly before main() return
        motordash.add(std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/bkgrd.eraw", rect))));
        for (auto i = ID_MIN, j =0; i <= ID_MAX; i += STEPPER, j++)
        {
            path.str("");
            path << "eraw/path" << std::to_string(i) << ".eraw";
            NeedleBase.push_back(std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize(path.str(), rect))));
            NeedleBase[j]->box(*rect);
            NeedleBase[j]->hide();
            motordash.add(NeedleBase[j]);
        }
        motordash.add_text_widget("#speed", "0", egt::Rect(298, 145, 200, 120), 120);
    };

#ifdef DO_SVG_SERIALIZATION
    if (need_serialization)
    {
        //If need serialization, show indicator for user on screen
        auto text = std::make_shared<egt::TextBox>("EGT is serializing, please wait...", egt::Rect(70, 190, 700, 200));
        text->border(0);
        text->font(egt::Font(50, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, egt::Palette::red);
        window.add(text);
        app.event().step();
        motordash.serialize_all();
        text->clear();
        text->text("Serialize successfully, welcome!");
        app.event().step();
        sleep(1);
        text->hide();
    }
#endif



#ifdef HAVE_LIBPLANES
    std::cout << "Have libplanes" << std::endl;
    //Create scale image to HEO overlay
    egt::Sprite scale_s(egt::Image(motordash.DeSerialize("eraw/moto_png.eraw", rect)), egt::Size(400, 240), 1, egt::Point(200, 120),
                           egt::PixelFormat::xrgb8888, egt::WindowHint::heo_overlay);
	std::cout << "Have libplanes" << std::endl;
    egt::detail::KMSOverlay* scale_ovl = reinterpret_cast<egt::detail::KMSOverlay*>(scale_s.screen());
	std::cout << "Have libplanes" << std::endl;
    plane_set_pan_size(scale_ovl->s(), 0, 0);
	std::cout << "Have libplanes" << std::endl;
    motordash.add(scale_s);
	std::cout << "Have libplanes" << std::endl;
    scale_s.show();
#endif

#ifdef SHOW_NEEDLE
    int old_needle_index = 0;
#endif
    bool is_needle_finish = true;
    std::string lbd_ret = "0";
    int timer_cnt;

	MOTO_STRUCT motoValue;

	auto threadCAN = [](MOTO_STRUCT& val) {		

		int s;
		int nbytes;
		struct sockaddr_can addr;
		struct ifreq ifr;
		struct can_frame frame;

		if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
			perror("Socket");
			return 1;
		}

		strcpy(ifr.ifr_name, "vcan0" );
		ioctl(s, SIOCGIFINDEX, &ifr);

		memset(&addr, 0, sizeof(addr));
		addr.can_family = AF_CAN;
		addr.can_ifindex = ifr.ifr_ifindex;

		if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("Bind");
			return 1;
		}

		std::cout << "Message from CAN Thread" << std::endl;
		val.needle_index = 0;


		while(1) {
			nbytes = read(s, &frame, sizeof(struct can_frame));

			if (nbytes < 0) {
				printf("Read");
				return 1;
			}

			val.needle_index = frame.data[0];
			val.speed_index = frame.data[1];
			val.head_light = (HEAD_LIGHT)frame.data[2];
			val.turn_light = (TURN_LIGHT)frame.data[3];
		}
	};

	std::thread tCan(threadCAN, std::ref(motoValue));

#if 1 //use timer or screen click to debug? 1:use timer; 0:use click
    egt::PeriodicTimer timer(std::chrono::milliseconds(100));
    timer.on_timeout([&]()
    {
        //motordash.get_high_q_state/get_low_q_state are used to protect high q not be interrupted by other event re-enter
        //if the state is false, it means this queue has not execute finished yet.
        if (!motordash.get_high_q_state())
            return;
        if (!high_pri_q.empty())
        {
            motordash.set_high_q_state(false);
            lbd_ret = high_pri_q.front()();
            high_pri_q.pop();
            motordash.set_high_q_state(true);
        }
        else
        {
            if (!motordash.get_low_q_state())
                return;
            if (!low_pri_q.empty())
            {
                motordash.set_low_q_state(false);
                lbd_ret = low_pri_q.front()();
                low_pri_q.pop();
                motordash.set_low_q_state(true);
                return;
            }
        }

        //viariable to control the animation and frequency
        timer_cnt = (LOW_Q_TIME_THRESHHOLD <= timer_cnt) ? 0 : timer_cnt + 1;


       //needle move function implemented by lambda
        auto needle_move = [&]()
        {
#ifdef SHOW_NEEDLE
			if( old_needle_index != motoValue.needle_index ) {
				if( motoValue.needle_index > old_needle_index ) {
					// increase
					for(int i=old_needle_index+1; i<=motoValue.needle_index; i++) {
						NeedleBase[i]->show();
					}
				} else {
					// decrease
					for(int i=old_needle_index; i>motoValue.needle_index; i--) {
						NeedleBase[i]->hide();
					}
				}
				
				motordash.find_text("#speed")->clear();
				motordash.find_text("#speed")->text(std::to_string(motoValue.needle_index));
				old_needle_index = motoValue.needle_index;

				is_needle_finish = true;
			}

#endif
			is_needle_finish = true;
            return "needle_move";
        };

#if 1
        //make sure the needles run a circle finish, then can enter to low priority procedure
        if (is_needle_finish)
		//if (true)
        {
            is_needle_finish = false;

#ifdef SHOW_HEADLIGHT
            if (!motordash.get_headlight_deserial_state())
            {
                low_pri_q.push([&]()
                {
					std::cout << "headlight deserialize" << std::endl;
                    farlightPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/farlight.eraw", rect)));
                    farlightPtr->box(*rect);
                    motordash.add(farlightPtr);
                    return "vsc_deserial";
                });
                motordash.set_headlight_deserial_state(true);
                return;
            }
            else
            {
                low_pri_q.push([&]()
                {
					if( motoValue.head_light == HEAD_LIGHT_OFF ) {
						farlightPtr->hide();
					} else {
						farlightPtr->show();
					}

                    return "farlight";
                });
            }
#endif // end of SHOW_HEADLIGHT

#ifdef SHOW_TURNLIGHT
			if (!motordash.get_turnlight_deserial_state())
            {
                low_pri_q.push([&]()
                {
					std::cout << "turnlight deserialize" << std::endl;
                    left_blinkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/left_blink.eraw", rect)));
                    left_blinkPtr->box(*rect);
                    motordash.add(left_blinkPtr);
                    right_blinkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/right_blink.eraw", rect)));
                    right_blinkPtr->box(*rect);
                    motordash.add(right_blinkPtr);
                    return "blink_deserial";
                });
                motordash.set_turnlight_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
						if( (motoValue.turn_light & 0xF0) == TURN_LIGHT_RIGHT ) {
							left_blinkPtr->show();
							if (right_blinkPtr->visible())
				                right_blinkPtr->hide();
				            else
				                right_blinkPtr->show();						
						}

						if( (motoValue.turn_light & 0xF) == TURN_LIGHT_LEFT ) {
							right_blinkPtr->show();
	                        if (left_blinkPtr->visible())
	                            left_blinkPtr->hide();
	                        else
	                            left_blinkPtr->show();
						}

						if( motoValue.turn_light == TURN_LIGHT_OFF ) {
							left_blinkPtr->show();
							right_blinkPtr->show();
						}

                        return "left_right_blink";
                    });
                }
            }
#endif // end of SHOW_TURNLIGHT
        }
        else  //this branch exec the high priority event
        {
            high_pri_q.push(needle_move);
        }


#endif
    });

#ifdef HAVE_LIBPLANES
    egt::PeriodicTimer scale_timer(std::chrono::milliseconds(16));
    scale_timer.on_timeout([&]()
    {
        //scale lambda function definition
        auto show_scale = [&]()
        {
            if (2.2 <= scale_factor)
            {
                is_scale_rev = true;
                is_scale_2_max = true;
#ifdef SCALE_TO_MAX_AND_STAY
                //gettimeofday(&time2, NULL);
                //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                //std::cout << "Scaling animation time: " << timediff << " us" << std::endl;
                is_scale_rev = false;
                is_scale_finish = true;
                scale_s.hide();
                scale_timer.stop();
                DeserialNeedles();
                timer.start();
                return;
#endif
            }
            if (0.001 >= scale_factor)
            {
                //gettimeofday(&time2, NULL);
                //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                //std::cout << "Scaling animation time: " << timediff << " us" << std::endl;
                is_scale_rev = false;
                is_scale_finish = true;
                scale_s.hide();
                scale_timer.stop();
                DeserialNeedles();
                timer.start();
                return;
            }
            plane_set_pos(scale_ovl->s(), PLANE_WIDTH_HF - plane_width(scale_ovl->s())*scale_factor/2, PLANE_HEIGHT_HF - plane_height(scale_ovl->s())*scale_factor/2);
            plane_set_scale(scale_ovl->s(), scale_factor);
            plane_apply(scale_ovl->s());
            scale_factor = is_scale_rev ? scale_factor - 0.08 : scale_factor + 0.08;
        };

        //scale one step in every time out
        if (!is_scale_finish)
        {
            show_scale();
            return;
        }
    });
    scale_timer.start();
#else
    DeserialNeedles();
    timer.start();
#endif

    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
            case egt::EventId::keyboard_up:
            {
                //std::cout << "click..." << std::endl;
                if (motordash.get_gear_deserial_state())
                {
                    //std::cout << "push gear to low q" << std::endl;
                    low_pri_q.push([&]()
                    {
                        return "click_gear";
                    });
                }
                break;
            }
            case egt::EventId::pointer_drag_start:
                //cout << "pointer_drag_start" << endl;

                break;
            case egt::EventId::pointer_drag_stop:

            break;
            case egt::EventId::pointer_drag:
            {
                //cout << "pointer_drag" << endl;
                break;
            }
            default:
                break;
        }
    };
    window.on_event(handle_touch);
#endif

    return app.run();
}

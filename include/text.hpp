#pragma once

#include <SDL2/SDL_ttf.h>

#include <map>
#include <string>
#include <memory>
#include <deque>

#include <include/renderable.hpp>
#include <include/color.hpp>
#include <include/rectangle_shape.hpp>
#include <include/time.hpp>

namespace ts
{
    /// \brief single or multi-line text
    class Text : public Renderable
    {
        public:
            /// \brief control sequence start/end
            static inline std::string tag_prefix = "<",
                    tag_suffix = ">",
                    tag_close_marker = "/";
            // example: <a> opens, </a> closes same region with tag a

            /// \brief basic format tags
            /// \note will be parsed as: \<tag_open_prefix>\<tag>\<tag_open_suffix> text \<tag_close_prefix>\<tag>\<tag_close_suffix>
            /// \example \<b>text\<\/b>
            static inline std::string bold_tag = "b",
                    italic_tag = "i",
                    underlined_tag = "u",
                    strikethrough_tag = "s";

            /// \brief color format tags, need to be followed by a color value
            /// \note will be parsed as: \<tag_open_prefix>\<tag=(r, g, b)> text \<tag_close_prefix>\<tag>\<tag_close_suffix><br>
            /// (spaces after comma optional. decimal pointers optional)
            /// \example \<col=(0.1, 0.8, 1)> text \</col><br>
            /// \<col=(0,1,1)> text \</col>
            static inline std::string color_foreground_tag = "col",
                                      color_background_tag = "col_bg";

            /// \brief initialize the text by loading a font
            /// \param font_size: size of font, in px
            /// \param font_familiy_name: arbitrary identifier of font, e.g. "Arial", "Roboto", etc.
            /// \param regular_path: absolute path to .ttf file containing the regular version of the font
            /// \param bold_path: [optional] absolute path to the .ttf file containing the bold version of the font
            /// \param italic_path: [optional] absolute path to the .ttf file containing the italic version of the font
            /// \param bold_italic_path: [optional] absolute path to the .ttf file containing the bold-italic version of the font
            /// \note if bold, italic and/or bold-italic are not specified, the regular font will be transform instead. Specifying separate
            //        fonts tends to lead to cleaner results
            Text(size_t font_size,
                const std::string& font_family_name,
                const std::string& regular_path,
                const std::string& bold_path = "",
                const std::string& italic_path = "",
                const std::string& bold_italic_path = "");

            /// \brief create the text, with wrapping
            /// \param render_target: render target in whose context the glyphs textures will be created
            /// \param formatted_text: text containing the format tags, will be parsed
            /// \param width: maximum width per line, or -1 for no wrapping
            /// \param line_spacer: vertical distance between lines, can be negative
            void create(Window&, Vector2f position, const std::string& formatted_text, size_t width_px = -1, int line_spacer = 1);

            /// \brief align the center of the texts bounding box with point
            /// \param point
            void set_centroid(Vector2f);

            /// \brief align top left of the first glyphs bounding box with point
            /// \param point
            void set_top_left(Vector2f);

            /// \brief get centroid of text bounding box
            /// \returns centroid
            Vector2f get_centroid() const;

            /// \brief get top left of bounding box
            /// \returns position
            Vector2f get_top_left() const;

            /// \brief get bounding box
            /// \returns bounding box
            Rectangle get_bounding_box() const;

            /// \brief get dimensions of bounding box
            /// \returns bounding box
            Vector2f get_size() const;

            /// \brief get number of lines, depends on wrapping
            /// \returns size_t
            size_t get_n_lines() const;

            /// \brief text alignment type
            enum AlignmentType
            {
                /// \brief aligned with left margin
                FLUSH_LEFT,

                /// \brief aligned with right margin
                FLUSH_RIGHT,

                /// \brief aligned such that there is an even gap on both margins
                CENTERED,

                /// \brief aligned such that there is no gap on both margins
                JUSTIFIED
            };

            /// \brief set text alignment
            /// \param type
            void set_alignment(AlignmentType);

            /// \brief set line spacing
            /// \param may_be_negative: spacing, in pixels. Negative values are allowed.
            void set_line_spacing(int may_be_negative);

            /// \brief set maximum width, or -1 for inifinite width
            /// \param width: in pixels
            void set_width(size_t);

            /// \brief align the north west center of entire text with point
            /// \param point
            void align_left_with(Vector2f);

            /// \brief align the center of first line with point
            /// \param point
            void align_center_with(Vector2f);

            /// \brief align north east center of enire text with poin
            /// \param point
            void align_right_with(Vector2f);

            /// \copydoc rat::Renderable::render
            void render(RenderTarget* target, Transform transform = Transform()) const override;

            /// \brief update the texts animations
            /// \param time: time elapsed since last frame, usually return value of `rat::Window::update`
            void update(Time);

        private:
            struct Font
            {
                TTF_Font* regular;
                TTF_Font* bold;
                TTF_Font* italic;
                TTF_Font* bold_italic;
            };

            static inline std::map<std::string, Font> _fonts;
            std::string _font_id;
            size_t _n_lines;
            Vector2f _position; // top left

            struct Glyph
            {
                Glyph(Window* target)
                    : _texture(target), _shape({0, 0}, {0, 0}), _background_shape({0, 0}, {0, 0})
                {}

                void set_top_left(Vector2f pos)
                {
                    pos.x = round(pos.x);
                    pos.y = round(pos.y);
                    _shape.set_top_left(pos);
                    _background_shape.set_top_left(pos);
                }

                bool _is_bold = false,
                     _is_italic = false,
                     _is_underlined = false,
                     _is_strikethrough = false;

                RGBA _foreground_color = RGBA(1, 1, 1, 1),
                     _background_color = RGBA(0, 0, 0, 0);

                std::string _content;
                StaticTexture _texture;
                RectangleShape _shape;
                RectangleShape _background_shape;
            };

            AlignmentType _alignment_type = FLUSH_LEFT;
            int _line_spacer = 1;
            size_t _width = -1;

            void apply_wrapping();
            std::deque<Glyph> _glyphs = {};
    };
}
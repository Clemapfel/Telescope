
#include <set>
#include <sstream>

#include <include/text.hpp>
#include <include/logging.hpp>

namespace ts
{
    Text::Text(size_t font_size,
        const std::string& font_family_name,
        const std::string& regular_path,
        const std::string& bold_path,
        const std::string& italic_path,
        const std::string& bold_italic_path)
        : _font_id(font_family_name)
    {
        if (not TTF_WasInit())
            TTF_Init();

        if (_fonts.find(font_family_name) == _fonts.end())
        {
            _fonts.emplace(font_family_name, Font{nullptr, nullptr, nullptr, nullptr});
            auto& element = _fonts.at(font_family_name);

            static auto load = [&](const std::string path) -> TTF_Font*
            {
                auto* out = TTF_OpenFont(path.c_str(), font_size);

                if (out == nullptr)
                {
                    Log::warning("In Text::Text: Unable to load font at ", path);
                    return nullptr;
                }

                return out;
            };

            element.regular = load(regular_path);
            element.bold = load(bold_path);
            element.italic = load(italic_path);
            element.bold_italic = load(bold_italic_path);
        }
    }

    void Text::render(RenderTarget* target, Transform transform) const
    {
        for (auto& glyph : _glyphs)
        {
            glyph._background_shape.render(target, transform);
        }

        for (auto& glyph : _glyphs)
        {
            glyph._shape.render(target, transform);
        }
    }

    void Text::create(Window& target, Vector2f position, const std::string& formatted_text, size_t width_px, int line_spacer)
    {
        if (_fonts.at(_font_id).regular == nullptr)
        {
            Log::warning("In Text::create: regular version of font not specified, unable to render any glyphs.");
            return;
        }

        _position = position;
        _width = width_px;
        _line_spacer = line_spacer;

        const auto original_position = position;
        size_t font_height = TTF_FontAscent(_fonts.at(_font_id).bold) + -1 * TTF_FontDescent(_fonts.at(_font_id).bold);

        bool bold_active = false,
             italic_active = false,
             underlined_active = false,
             strikethrough_active = false,
             color_foreground_active = false,
             color_background_active = false;

        RGBA foreground = RGBA(1, 1, 1, 1),
             background = RGBA(0, 0, 0, 0);

        const auto& font = _fonts.at(_font_id);

        // exclude certain ascii chars
        std::set<uint8_t> do_not_render = {
                '\t',  // tab
                '\n',  // newline
                uint8_t(256 - 83) // soft hyphen acts weird on some OS
        };

        for (uint8_t i = 0; i <= 31; ++i) // deprecated control chars
            do_not_render.insert(i);

        for (uint8_t i = 127; i <= 160; ++i) // extended control chars
            do_not_render.insert(i);

        auto push_glyph = [&](const std::string& raw, Vector2f position)
        {
            for (auto& c : raw)
                if (do_not_render.find(c) != do_not_render.end())
                    return;

            _glyphs.emplace_back(&target);
            auto& glyph = _glyphs.back();
            glyph._is_bold = bold_active;
            glyph._is_italic = italic_active;
            glyph._is_underlined = underlined_active;
            glyph._is_strikethrough = strikethrough_active;
            glyph._foreground_color = foreground;
            glyph._background_color = background;
            glyph._content = raw;

            TTF_Font* current_font = nullptr;
            uint32_t style = TTF_STYLE_NORMAL;

            // use specialized font unless unavailable
            if (bold_active and italic_active)
            {
                if (font.bold_italic == nullptr)
                {
                    current_font = font.regular;
                    style |= TTF_STYLE_BOLD;
                    style |= TTF_STYLE_ITALIC;
                }
                else
                    current_font = font.bold_italic;
            }
            else if (bold_active)
            {
                if (font.bold == nullptr)
                {
                    current_font = font.regular;
                    style |= TTF_STYLE_BOLD;
                }
                else
                    current_font = font.bold;
            }
            else if (italic_active)
            {
                if (font.italic == nullptr)
                {
                    current_font = font.regular;
                    style |= TTF_STYLE_ITALIC;
                }
                else
                    current_font = font.italic;
            }
            else
                current_font = font.regular;

            if (underlined_active)
                style |= TTF_STYLE_ITALIC;

            if (strikethrough_active)
                style |= TTF_STYLE_STRIKETHROUGH;

            TTF_SetFontStyle(current_font, style);
            SDL_Surface* surface = nullptr;

            surface = TTF_RenderText_Blended(current_font, raw.data(), foreground.operator SDL_Color());
            if (surface == nullptr or surface->w == 0 or surface->h == 0)
            {
                std::cerr << "[WARNING] In Text::create: Unable to render char `" << raw << "` (" << int(raw.back()) << ")" << std::endl;
                _glyphs.pop_back();
                return;
            }

            glyph._texture.create_from(surface);
            position.y -= (font_height * 0.5);

            glyph._shape = RectangleShape(position, Vector2f(surface->w, surface->h));
            glyph._shape.set_color(glyph._foreground_color);
            glyph._background_shape = RectangleShape(position, Vector2f(surface->w, surface->h));
            glyph._background_shape.set_color(glyph._background_color);

            // reset
            SDL_FreeSurface(surface);
            for (auto* f : {font.regular, font.bold, font.italic, font.bold_italic})
            {
                if (f != nullptr)
                    TTF_SetFontStyle(f, TTF_STYLE_NORMAL);
            }
        };

        auto parse_color = [](const std::string& str) -> RGBA
        {
            // assumes string of the type (r,g,b) or (r, g, b)
            if (str.at(0) != '(')
            {
                std::string msg = "Error parsing color: got: `(`, got: `";
                msg.push_back(str.at(0));
                msg.push_back('`');
                throw std::invalid_argument(msg);
            }

            std::stringstream red, green, blue;

            size_t comma_count = 0;
            for (size_t i = 1; i < str.size(); ++i)
            {
                if (str.at(i) == ',')
                    comma_count++;
                else if (str.at(i) == ' ')
                    continue;
                else
                {
                    if (comma_count == 0)
                        red << str.at(i);
                    else if (comma_count == 1)
                        green << str.at(i);
                    else if (comma_count == 2)
                        blue << str.at(i);
                    else if (comma_count == 3)
                    {
                        static bool once = true;
                        if (once)
                        {
                            std::cerr << "[WARNING] In Text::create::parse_color: Any color component other than r, g and b will be ignored." << std::endl;
                            once = false;
                        }
                    }
                    else
                        throw std::invalid_argument("In Text::create::parse_color: Unable to parse color: too many color components");
                }
            }

            auto r = std::stof(red.str());
            auto g = std::stof(green.str());
            auto b = std::stof(blue.str());

            if (r < 0 or g < 0 or b < 0 or r > 1 or g > 1 or b > 1)
                std::cerr << "[WARNING] In Text::create::parse_color: Color components should be 32-bit floats in [0, 1], got: " << str << ")" << std::endl;

            return RGBA(r, g, b, 1);
        };

        auto substr_is = [&](size_t i, const std::string& str) -> bool
        {
            return formatted_text.substr(i, str.size()) == str;
        };

        auto assert_not_open = [](bool b) -> void
        {
            if (b)
                throw std::invalid_argument("trying to open a region that is already open");
        };

        auto assert_open = [](bool b) -> void
        {
            if (not b)
                throw std::invalid_argument("trying to close a region that is already closed");
        };

        size_t i = 0;
        try
        {
            while (i < formatted_text.size())
            {
                if (substr_is(i, "\\"))
                {
                    i += 1;
                    goto skip_control_tags;
                }

                // start open
                if (substr_is(i, tag_prefix) and not substr_is(i+1, tag_close_marker))
                {
                    i += tag_prefix.size();
                    if (substr_is(i, bold_tag) and substr_is(i + bold_tag.size(), tag_suffix))
                    {
                        assert_not_open(bold_active);

                        bold_active = true;
                        i += bold_tag.size();
                    }
                    else if (substr_is(i, italic_tag) and substr_is(i + italic_tag.size(), tag_suffix))
                    {
                        assert_not_open(italic_active);

                        italic_active = true;
                        i += italic_tag.size();
                    }
                    else if (substr_is(i, underlined_tag) and substr_is(i + underlined_tag.size(), tag_suffix))
                    {
                        assert_not_open(underlined_active);

                        underlined_active = true;
                        i += underlined_tag.size();
                    }
                    else if (substr_is(i, strikethrough_tag) and substr_is(i + strikethrough_tag.size(), tag_suffix))
                    {
                        assert_not_open(strikethrough_active);

                        strikethrough_active = true;
                        i += strikethrough_tag.size();
                    }
                    else if (substr_is(i, color_foreground_tag) and substr_is(i + color_foreground_tag.size(), "="))
                    {
                        assert_not_open(color_foreground_active);
                        color_foreground_active = true;

                        i += color_foreground_tag.size();
                        if (not substr_is(i, "="))
                        {
                            std::stringstream str;
                            str << "Expected: `=` after `" << color_foreground_tag << "`. Got: " << formatted_text.at(i);
                            throw std::invalid_argument(str.str());
                        }

                        i += 1;

                        std::stringstream str;
                        while (not substr_is(i, ")"))
                        {
                            str << formatted_text.at(i);
                            i += 1;

                            if (i >= formatted_text.size() - 1)
                                throw std::invalid_argument("Unable to parse color tag value");
                        }
                        foreground = parse_color(str.str());
                        i += 1;
                    }
                    else if (substr_is(i, color_background_tag) and substr_is(i + color_background_tag.size(), "="))
                    {
                        assert_not_open(color_background_active);
                        color_background_active = true;

                        i += color_background_tag.size();
                        if (not substr_is(i, "="))
                        {
                            std::stringstream str;
                            str << "Expected: `=` after `" << color_background_tag << "`. Got: " << formatted_text.at(i);
                            throw std::invalid_argument(str.str());
                        }
                        i += 1;

                        std::stringstream str;
                        while (not substr_is(i, ")"))
                        {
                            str << formatted_text.at(i);
                            i += 1;

                            if (i >= formatted_text.size() - 1)
                                throw std::invalid_argument("Unable to parse color tag value");
                        }
                        background = parse_color(str.str());
                        i += 1;
                    }
                    else
                        throw std::invalid_argument("unrecognized control tag");

                    if (not substr_is(i, tag_suffix))
                    {
                        std::stringstream str;
                        str << "Expected: " << tag_suffix << ". Got: " << formatted_text.at(i);
                        throw std::invalid_argument(str.str());
                    }

                    i += tag_suffix.size();
                    continue;
                }

                    // start close
                else if (substr_is(i, tag_prefix) and substr_is(i+1, tag_close_marker))
                {
                    i += tag_prefix.size();
                    i += tag_close_marker.size();

                    if (substr_is(i, bold_tag) and substr_is(i + bold_tag.size(), tag_suffix))
                    {
                        assert_open(bold_active);

                        bold_active = false;
                        i += bold_tag.size();
                    }
                    else if (substr_is(i, italic_tag) and substr_is(i + italic_tag.size(), tag_suffix))
                    {
                        assert_open(italic_active);

                        italic_active = false;
                        i += italic_tag.size();
                    }
                    else if (substr_is(i, underlined_tag) and substr_is(i + underlined_tag.size(), tag_suffix))
                    {
                        assert_open(underlined_active);

                        underlined_active = false;
                        i += underlined_tag.size();
                    }
                    else if (substr_is(i, strikethrough_tag) and substr_is(i + strikethrough_tag.size(), tag_suffix))
                    {
                        assert_open(strikethrough_active);

                        strikethrough_active = false;
                        i += underlined_tag.size();
                    }
                    else if (substr_is(i, color_foreground_tag) and substr_is(i + color_foreground_tag.size(), tag_suffix))
                    {
                        assert_open(color_foreground_active);

                        color_foreground_active = false;
                        foreground = RGBA(1, 1, 1, 1);

                        i += color_foreground_tag.size();
                    }
                    else if (substr_is(i, color_background_tag) and substr_is(i + color_background_tag.size(), tag_suffix))
                    {
                        assert_open(color_background_active);

                        color_background_active = false;
                        background = RGBA(0, 0, 0, 0);

                        i += color_background_tag.size();
                    }
                    else
                        throw std::invalid_argument("unrecognized control tag");

                    if (not substr_is(i, tag_suffix))
                    {
                        std::stringstream str;
                        str << "Expected: `" << tag_suffix << "`, Got: `" << formatted_text.at(i) << "`";
                        throw std::invalid_argument(str.str());
                    }

                    i += tag_suffix.size();
                    continue;
                }

                skip_control_tags:

                if (i >= formatted_text.size())
                    break;

                std::string to_push;
                to_push.push_back(formatted_text.at(i));
                push_glyph(to_push, position);

                i += 1;

                if (not _glyphs.empty())
                    position.x += _glyphs.back()._shape.get_size().x;
            }
        }
        catch (std::invalid_argument& exc)
        {
            // pretty printing parsing error

            std::cerr << "[ERROR] In Text::create: Error parsing text at position " << i << std::endl;

            size_t count = 0;
            const size_t left_offset = 20;
            const size_t right_offset = 10;

            std::cerr << "\t";
            for (size_t j = std::max<int>(int(i) - left_offset, 0); j <= i + right_offset && j < formatted_text.size(); ++j, count++)
                std::cerr << formatted_text.at(j);

            std::cerr << std::endl;

            std::cerr << "\t";
            for (size_t j = 0; j <= count - right_offset; ++j)
                std::cerr << "~";

            std::cerr << "^" << std::endl;
            std::cerr << " what(): " << exc.what() << std::endl;
        }

        auto warn_never_closed = [](const std::string& which, const std::string& tag) {
            std::cerr << "[WARNING] In Text::create: Reached end of text, but "
                      << which << " region was never closed with "
                      << tag_prefix << tag_close_marker << tag << tag_suffix
                      << std::endl;
        };

        if (bold_active)
            warn_never_closed("bold", bold_tag);

        if (italic_active)
            warn_never_closed("italic", italic_tag);

        if (strikethrough_active)
            warn_never_closed("strikethrough", strikethrough_tag);

        if (underlined_active)
            warn_never_closed("underlined", underlined_tag);

        if (color_foreground_active)
            warn_never_closed("color", color_foreground_tag);

        if (color_background_active)
            warn_never_closed("background color", color_background_tag);

        for (auto& g : _glyphs)
            g._shape.set_texture(&g._texture);

        apply_wrapping();

        for (auto& g : _glyphs)
        {
            g._background_shape = g._shape;
            g._background_shape.set_texture(nullptr);
            g._background_shape.set_color(g._background_color);
            g._background_shape.set_color(g._background_color);
        }
    }

    void Text::apply_wrapping()
    {
        if (_glyphs.empty())
            return;

        const auto original_position = _position;

        auto is_delimiter_char = [](char in) -> bool
        {
            for (auto c : {' ', ',', '.', ';', '\t', '\n'})
                if (in == c)
                    return true;

            return false;
        };

        const auto line_height = TTF_FontAscent(_fonts.at(_font_id).bold) + _line_spacer;

        auto position = original_position;

        float current_line_width = 0;
        _n_lines = 1; // sic
        for (size_t i = 0; i < _glyphs.size(); ++i)
        {
            std::vector<Glyph*> word;
            float word_width = 0;
            while ((i < _glyphs.size()))
            {
                word.push_back(&_glyphs.at(i));

                auto glyph_width = _glyphs.at(i)._shape.get_size().x;
                word_width += glyph_width;

                if (is_delimiter_char(_glyphs.at(i)._content.back()))
                {
                    word_width -= word.back()->_shape.get_size().x;
                    break;
                }
                i += 1;
            }

            // put word on next line if it doesn't fit
            if ((current_line_width + word_width > _width))
            {
                position.y += line_height;
                _n_lines += 1;
                position.x = original_position.x;
                current_line_width = 0;
            }

            for (auto* glyph : word)
            {
                glyph->set_top_left(position);
                const auto glyph_width = glyph->_shape.get_size().x;
                current_line_width += glyph_width;
                position.x += glyph_width;
            }

            // hard wrap next line
            if (word.back()->_content.back() == '\n')
            {
                position.y += line_height;
                _n_lines += 1;
                position.x = original_position.x;
                current_line_width = 0;
            }
        }

        if (_alignment_type == FLUSH_LEFT)
            return;

        int width = 0, height = 0;
        TTF_SizeText(_fonts.at(_font_id).bold, " ", &width, &height);
        const int width_of_space = width;

        if (_alignment_type == FLUSH_RIGHT)
        {
            float right_x = std::numeric_limits<float>::min();
            for (auto& glyph : _glyphs)
                right_x = std::max(right_x, glyph._shape.get_top_left().x);

            for (size_t i = 0; i < _glyphs.size(); ++i)
            {
                auto line = std::vector<Glyph*>{&_glyphs.at(i)};
                const float line_y = line.front()->_shape.get_top_left().y;

                i += 1;
                while (i < _glyphs.size() and _glyphs.at(i)._shape.get_top_left().y == line_y)
                {
                    line.push_back(&_glyphs.at(i));
                    i += 1;
                }
                i -= 1;

                auto& last_glyph = line.back();
                float offset = right_x - (last_glyph->_shape.get_top_left().x + last_glyph->_shape.get_size().x);

                if (last_glyph->_content.back() == ' ')
                    offset += width_of_space;

                for (auto* glyph : line)
                    glyph->set_top_left(glyph->_shape.get_top_left() + Vector2f(offset, 0));
            }

            return;
        }

        if (_alignment_type == CENTERED)
        {
            auto aabb = get_bounding_box();
            auto text_center = aabb.top_left.x + aabb.size.x * 0.5;

            for (size_t i = 0; i < _glyphs.size(); ++i)
            {
                auto line = std::vector<Glyph *>{&_glyphs.at(i)};
                const float line_y = line.front()->_shape.get_top_left().y;

                i += 1;
                while (i < _glyphs.size() and _glyphs.at(i)._shape.get_top_left().y == line_y)
                {
                    line.push_back(&_glyphs.at(i));
                    i += 1;
                }
                i -= 1;

                auto line_left = line.front()->_shape.get_top_left().x;
                auto line_right = line.back()->_shape.get_top_left().x + line.back()->_shape.get_size().x;

                if (line.back()->_content.back() == ' ')
                    line_right -= width_of_space;

                auto line_center = line_left + (line_right - line_left) * 0.5;
                auto offset = text_center - line_center;

                for (auto* glyph : line)
                    glyph->set_top_left(glyph->_shape.get_top_left() + Vector2f(offset, 0));
            }

            return;
        }

        if (_alignment_type == JUSTIFIED)
        {
            auto aabb = get_bounding_box();
            auto text_left = aabb.top_left.x;
            auto text_center = aabb.top_left.x + aabb.size.x * 0.5;
            auto text_right = aabb.top_left.x + aabb.size.x;

            for (size_t i = 0; i < _glyphs.size(); ++i)
            {
                auto line = std::vector<Glyph *>{&_glyphs.at(i)};
                const float line_y = line.front()->_shape.get_top_left().y;

                i += 1;
                while (i < _glyphs.size() and _glyphs.at(i)._shape.get_top_left().y == line_y)
                {
                    line.push_back(&_glyphs.at(i));
                    i += 1;
                }
                i -= 1;

                std::deque<std::vector<Glyph*>> words = {};
                std::deque<float> word_lengths;

                words.emplace_back();
                word_lengths.push_back(0);

                for (size_t i = 0; i < line.size(); ++i)
                {
                    auto* glyph = line.at(i);

                    words.back().push_back(glyph);
                    word_lengths.back() += glyph->_shape.get_size().x;

                    if (glyph->_content.back() == ' ')
                    {
                        word_lengths.back() -= width_of_space;

                        if (i < line.size()-1)
                        {
                            word_lengths.push_back(0);
                            words.emplace_back();
                        }
                    }
                }

                // first word left already aligned left
                // align last word right
                Vector2f position = Vector2f(text_right - word_lengths.back(), words.back().front()->_shape.get_top_left().y);
                for (auto* g : words.back())
                {
                    g->set_top_left(position);
                    position.x += g->_shape.get_size().x;
                }

                // calculate spacing for other words
                float free_space = aabb.size.x;
                for (auto w : word_lengths)
                    free_space -= w;

                words.pop_front();
                words.pop_back();

                if (words.empty())
                    continue;

                float free_space_per_word = free_space / (words.size() + 1);
                position.x = text_left + word_lengths.front() + free_space_per_word;

                for (auto& word : words)
                {
                    for (auto* glyph : word)
                    {
                        glyph->set_top_left(position);
                        position.x += glyph->_shape.get_size().x;
                        if (glyph->_content.back() == ' ')
                            position.x -= width_of_space;
                    }

                    position.x += free_space_per_word;
                }
            }
        }
    }

    void Text::set_alignment(AlignmentType type)
    {
        if (_alignment_type != type)
        {
            _alignment_type = type;

            if (not _glyphs.empty())
                apply_wrapping();
        }
    }

    void Text::set_line_spacing(int may_be_negative)
    {
        if (_line_spacer != may_be_negative)
        {
            _line_spacer = may_be_negative;
            if (not _glyphs.empty())
                apply_wrapping();
        }
    }

    void Text::set_width(size_t width)
    {
        if (_width != width)
        {
            _width = width;
            if (not _glyphs.empty())
                apply_wrapping();
        }
    }

    void Text::update(Time time)
    {}

    Rectangle Text::get_bounding_box() const
    {
        auto out = Rectangle();
        auto first_glyph = _glyphs.at(0)._shape;

        // x bounds
        float max_x = std::numeric_limits<float>::min();
        float min_x = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::min();
        float min_y = std::numeric_limits<float>::max();

        for (auto& glyph : _glyphs)
        {
            max_x = std::max(max_x, glyph._shape.get_top_left().x + glyph._shape.get_size().x);
            min_x = std::min(min_x, glyph._shape.get_top_left().x);
            max_y = std::max(max_y, glyph._shape.get_top_left().y + glyph._shape.get_size().y);
            min_y = std::min(min_y, glyph._shape.get_top_left().y);
        }

        out.size.x = max_x - min_x;
        out.size.y = max_y - min_y;
        out.top_left.x = min_x;
        out.top_left.y = min_y;

        return out;
    }

    Vector2f Text::get_size() const
    {
        return get_bounding_box().size;
    }

    void Text::set_top_left(Vector2f position)
    {
        auto offset = position - _glyphs.at(0)._shape.get_top_left();
        _position += offset;
        for (auto& glyph : _glyphs)
        {
            auto current_pos = glyph._shape.get_top_left();
            glyph.set_top_left(current_pos + offset);
        }
    }

    Vector2f Text::get_top_left() const
    {
        return _glyphs.at(0)._shape.get_top_left();
    }

    void Text::set_centroid(Vector2f position)
    {
        const auto aabb = get_bounding_box();
        auto offset = position - (aabb.top_left + aabb.size * Vector2f(0.5, 0.5));
        _position += offset;

        for (auto& glyph : _glyphs)
        {
            auto current_pos = glyph._shape.get_top_left();
            glyph.set_top_left(current_pos + offset);
        }
    }

    Vector2f Text::get_centroid() const
    {
        const auto aabb = get_bounding_box();
        return aabb.top_left + aabb.size * Vector2f(0.5, 0.5);
    }

    size_t Text::get_n_lines() const
    {
        return _n_lines;
    }

    void Text::align_left_with(Vector2f point)
    {
        auto aabb = get_bounding_box();
        auto center = get_centroid();

        auto offset = point - center;
        offset.x += aabb.size.x * 0.5;
        _position += offset;
        for (auto &glyph: _glyphs)
        {
            auto current_pos = glyph._shape.get_top_left();
            glyph.set_top_left(current_pos + offset);
        }
    }

    void Text::align_center_with(Vector2f point)
    {
        auto center = get_centroid();
        auto offset = point - center;
        _position += offset;

        for (auto &glyph: _glyphs)
        {
            auto current_pos = glyph._shape.get_top_left();
            glyph.set_top_left(current_pos + offset);
        }
    }

    void Text::align_right_with(Vector2f point)
    {
        auto aabb = get_bounding_box();
        auto center = get_centroid();
        auto offset = point - center;
        offset.x -= aabb.size.x * 0.5;
        _position += offset;

        for (auto &glyph: _glyphs)
        {
            auto current_pos = glyph._shape.get_top_left();
            glyph.set_top_left(current_pos + offset);
        }
    }
}
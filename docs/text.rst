Text
====

Text is central to communicating with the user of a graphical application. Unfortunately, many engines only
provide a bare-bones text implementation of renderings texts that's little more than a texture of a glyph.

.. note::
    A glyph is a symbol in a true type font (.ttf). For example, both :code:`'A'` and :code:`' '` (the space character) are
    glyphs that are part of almost every font.

Telescopes text engine provides alignment modes, pixel-perfect positioning and a familiar formatting language,
making prettifying text in any context simple.

Format Tags
^^^^^^^^^^^

Telescope uses a html-like tag system. By using special tag characters, we can hint to telescope to format any character
in that region a specific way.

Let :code:`x` be a format tag. To open a region with this tag, we use :code:`<x>`. To close the region, we use :code:`</x>`. We cannot
open a region that is already open, or close a region that is already closed, otherwise a parser error is issued.

The available tags are as follows:

Bold: :code:`b`
***************

Example: :code:`<b>text</b>`

**bold**, or emphasized text

Italic: :code:`i`
*****************

Example: :code:`<i>text</i>`

*italic* text

Underlined: :code:`u`
**********

Example: :code:`<u>text</u>`

Underlined text, a horizontal line along the base of the text, has the same color and weight as the text.

Strikethrough: :code:`s`
************************

Example: :code:`<s>text</s>`

Strikethrough text, a horizontal line through the center of the text, has the same color and weight as the text.

Foreground Color: :code:`col`
*****************************

Example: :code:`<col=(1, 0, 0)>text</col>`

Color the glyphs themself. :code:`col` needs to be followed by a color in rgb format, where each component is in :code:`[0, 1]`. For example, :code:`<col=(1, 0, 0)>` opens a region of red text, :code:`<col=(0, 0, 0)>` opens a region of black text. Only one foreground color region can be active at one time. The round brackets are not optional.

Background Color: :code:`col_bg`
********************************

Example: :code:`<col_bg=(1, 0, 1)>text</col_bg>`

Color the background bounds of the glyph, the glyph itself is unmodified. Similar to :code:`col`, needs to be followed by a color in rgb format. Only one background and one foreground color respectively can be active at the same time.

Escape Character
****************

If we want any of the characters used as part of the tag syntax, we can simply follow them with a :code:`\`. For example, :code:`<b>text</b>` will render as as bold "**text**". :code:`\\<b>text\\</b>` will render as non-bold (regular) "<b>text</b>".

----------------------

Unlike with other languages such as markdown or .rst, in telescope, format regions may overlap. For example, the text
:code:`<b>first<i> second</b></i>` will be parsed such that :code:`first` is bold while :code:`second` is both bold and italic,
as the bold region was only closed after.

Color tags require an :code:`=` followed by a :code:`(r,g,b)`, where r, g, b are floats in :code:`[0, 1]`. This specifies the color. Spaces around the commas are optionally, if a
fourh alpha component is specified, it will only issue a soft warning but be ignored for actually formatting the text.

----------------------

Loading Fonts
^^^^^^^^^^^^^

Now that we know how to write strings that telescope can pass into formatted text, we should talk about the class that makes this possible, :code:`ts::Text`:

.. doxygenclass:: ts::Text

Before any parsing can occurr, we need to initialize the text object using its only constructor:

.. doxygenfunction:: ts::Text::Text

The constructor takes information about the font class we are going to be using for our text. .ttf files are complex and their
intricacies are beyond the scope of this documentation, however, we should be aware of the following properties of a .ttf files context:

+ **font family**: cleartext name of the font, for example "Roboto" or "DejaVuSans Mono"
+ **size**: height of the average glyph, in pixels
+ **weight**: weight of the font, governs the thickness of glyphs lines. For our purposes we want to pick two different thickness: *regular* and *bold*
+ **being italic**: most font families offer a separate file for the italic version of a font.

For example, if we wanted to use the font family Roboto at size 48px, available `here <https://fonts.google.com/specimen/Roboto>`_, we would download four files:

+ :code:`Roboto-Regular.ttf`: font version to be user if no format tag is active
+ :code:`Roboto-Bold.ttf`: font version to be used if the bold tag is active and the italic tag is not active
+ :code:`Roboto-Italic.ttf`: font version to be used if the italic tag is active and the bold tag is not active
+ :code:`Roboto-BoldItalic.ttf`: font version to be used if both the bold and italic tag are active

With these four files, we can initialize our text object like so:

.. code-block:: cpp
    :caption: Creating a Text Object

    auto text = ts::Text(
        48,         // native size
        "Roboto",   // font family id
        "path/to/Roboto-Regular.ttf",   // absolute path to regular version
        "path/to/Roboto-Bold.ttf",      // absolute path to bold version
        "path/to/Roboto-Italic.ttf",    // absolute path to italic version
        "path/to/Roboto-BoldItalic.ttf" // absolute path to bold-italic version
    )

This is the recommended way of creating text objects, as it results in the cleanest renderings. If we specify a different
font size from that used in the .ttf font, the text will render, but artifacting may occur. The bold, italic and bold-italic
version of the fonts are also optional, though, again, by not specifying these separately, the rendered text may be less
smooth and exhibit artifacting such as jagged lines.

If a font with the same font family was already loaded, the font will not be allocated again. Telescope can tell fonts
apart only by their font family id specified during the constructor. There is no mechanism in place to verify that all four
fonts are actually of the same font family.

Creating the Text
^^^^^^^^^^^^^^^^^

With the text object aware of which font and font size we want to use, we can start the parsing process using

.. doxygenfunction:: ts::Text::create


This function takes a window, which provides the render context for he glyphs textures, the formatted text as a string (including the tags), the texts position, and
two more parameters:

+ **width** is the maximum width of the text. This will govern the wrapping behavior of the text. Setting width to -1 will disable wrapping, making it such that the entire text is on only one line
+ **line_spacer** is an optional parameter that will add an pixel offset at the bottom of each line, increasing the space between each line. We can even choose a negative values,
if we want the lines to be closer together, though this will usually result in lines overlapping

.. code-block:: cpp

    auto window = Window( // ...

    // initialize the text and font
    auto text = ts::Text(
        48,         // native size
        "Roboto",   // font family id
        "path/to/Roboto-Regular.ttf",   // absolute path to regular version
        "path/to/Roboto-Bold.ttf",      // absolute path to bold version
        "path/to/Roboto-Italic.ttf",    // absolute path to italic version
        "path/to/Roboto-BoldItalic.ttf" // absolute path to bold-italic version
    )

    // create the text
    auto formatted_string = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur...";
    text.create(
        window,     // render context
        {0, 0},     // initial position
        formatted_string,   // raw formatted string
        window.get_size().x - (2 * 50));  // width for wrapping

    // align the text with the center of the window
    text.set_centroid(Vector2f(window.get_size().x * 0.5, window.get_size().y * 0.5));

    // render loop
    while (window.is_open())
    {
        auto time = start_frame(&window);

        window.render(&text);

        end_frame(&window);
    }

.. image:: _static/text_lipsum_flush_left.png

----------------------

Text Alignment
^^^^^^^^^^^^^^

**Alignment** governs how words are distributed on each line of the text. There are four types of alignment:

.. doxygenenum:: ts::Text::AlignmentType

Using :code:`ts::Text::set_alignment`, we can set the texts alignment - before or after :code:`create` was called. By
default, a texts alignment is set to :code:`Text::FLUSH_LEFT`.

FLUSH_LEFT
**********

.. figure:: _static/text_lipsum_flush_left.png
    :scale: 50%

FLUSH_RIGHT
***********

.. figure:: _static/text_lipsum_flush_right.png
    :scale: 50%

CENTERED
********

.. figure:: _static/text_lipsum_centered.png
    :scale: 50%

JUSTIFIED
*********

.. figure:: _static/text_lipsum_justified.png
    :scale: 50%

-------------------

Accessing Individual Glyphs
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Internally, a text is a collection of rectangle shapes whose texture is the glyph to be displayed. The background color of a glyph is a separate rectangle shape,
un-textured, with its vertices the color of the glyphs background color. Background shapes have the color :code:`RGBA(0, 0, 0, 0)` by default,
making them transparent and thus invisible.

We can access both these shapes directly using :code:`ts::Text::get_glyph_shape` and :code:`ts::Text::get_glyph_background_shape`.
This allows user to modify them freely.

For example:

.. code-block:: cpp
    :caption: coloring each glyph after the text was created

    auto window = // ...

    auto formatted_string = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur...";
    auto text = Text(48, "Roboto",
         "Roboto-Regular.ttf",
         "Roboto-Bold.ttf",
         "Roboto-Italic.ttf",
         "Roboto-BoldItalic.ttf"
    );
    text.create(window, {0, 0}, formatted_string, window.get_size().x - 2 * 50);
    text.set_centroid(Vector2f(window.get_size().x * 0.5, window.get_size().y * 0.5));
    text.set_alignment(Text::JUSTIFIED);

    // iterate through glyph shapes and give each a linearly increasing hue
    size_t n = text.get_n_glyphs();
    for (size_t i = 0; i < n; ++i)
        text.get_glyph_shape(i)->set_color(HSVA(float(i) / n, 1, 1, 1));

.. image:: _static/text_lipsum_rainbow.png

-------------------------

ts::Text
^^^^^^^^

A full list of all members and member function of :code:`ts::Text` is provided here.

.. doxygenclass:: ts::Text
    :members:

.. doxygenstruct:: ts::Font
    :members:















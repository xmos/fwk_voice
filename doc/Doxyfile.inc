# This file provides overrides to the Doxyfile configuration

PROJECT_NAME = Avona
PROJECT_BRIEF = "Avona Reference Design"

# AEC module
INPUT += ../modules/lib_aec/api ../examples/bare-metal/shared_src/aec/

# AGC module
INPUT += ../modules/lib_agc/api

USE_MATHJAX            = YES
MATHJAX_FORMAT         = HTML-CSS
MATHJAX_RELPATH        = https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/

# Make short MATHJAX sequences prettier
ALIASES += math{1}="@f$\1@f$"

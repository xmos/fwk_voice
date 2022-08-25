# This file provides overrides to the Doxyfile configuration

PROJECT_NAME = Avona
PROJECT_BRIEF = "Avona Reference Design"

# AEC module
INPUT += ../modules/lib_aec/api ../examples/bare-metal/shared_src/aec/

# NS module
INPUT += ../modules/lib_ns/api

# AGC module
INPUT += ../modules/lib_agc/api

# ADEC module
INPUT += ../modules/lib_adec/api

# IC module
INPUT += ../modules/lib_ic/api

# VNR module
INPUT += ../modules/lib_vnr/api/common
INPUT += ../modules/lib_vnr/api/features
INPUT += ../modules/lib_vnr/api/inference

PREDEFINED             = C_API= EXTERN_C= DWORD_ALIGNED=

USE_MATHJAX            = YES
MATHJAX_FORMAT         = HTML-CSS
MATHJAX_RELPATH        = https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/

# Make short MATHJAX sequences prettier
ALIASES += math{1}="@f$\1@f$"

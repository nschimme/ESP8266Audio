#ifndef CONFIG_HELIX_H
#define CONFIG_HELIX_H

/// Configuration options for Helix AAC decoder

/// On ESP8266: enable Downsampled SBR (HE-AAC v1), but disable PS (HE-AAC v2) to save RAM
#ifdef ESP8266
#  ifndef AAC_ENABLE_SBR
#    define AAC_ENABLE_SBR 1
#  endif
#  ifndef AAC_ENABLE_SBR_DOWNSAMPLED
#    define AAC_ENABLE_SBR_DOWNSAMPLED 1
#  endif
#else
#  ifndef AAC_ENABLE_SBR
#    define AAC_ENABLE_SBR 1
#  endif
#  ifndef AAC_ENABLE_PS
#    define AAC_ENABLE_PS 1
#  endif
#  ifndef AAC_ENABLE_SBR_DOWNSAMPLED
#    define AAC_ENABLE_SBR_DOWNSAMPLED 0
#  endif
#endif

#endif /* CONFIG_HELIX_H */

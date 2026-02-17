#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

/// Color token constants for the Nebula design system.
/// All UI components read from these — never hardcode colors.
namespace NebulaColours {
// --- Background layers ---
inline const juce::Colour bg_deepest{0xFF0A0A14}; // Darkest panel
inline const juce::Colour bg_panel{0xFF12121F};   // Standard panel
inline const juce::Colour bg_surface{0xFF1A1A2E}; // Elevated surface
inline const juce::Colour bg_hover{0xFF222240};   // Hovered element

// --- Accent - Indigo ---
inline const juce::Colour accent1{0xFF6366F1};      // Primary accent
inline const juce::Colour accent1_dim{0xFF4338CA};  // Dimmed variant
inline const juce::Colour accent1_glow{0x806366F1}; // Glow (50% alpha)

// --- Accent - Pink ---
inline const juce::Colour accent2{0xFFF472B6};      // Secondary accent
inline const juce::Colour accent2_dim{0xFFDB2777};  // Dimmed
inline const juce::Colour accent2_glow{0x80F472B6}; // Glow

// --- Text ---
inline const juce::Colour text_bright{0xFFF1F5F9}; // Headers, values
inline const juce::Colour text_normal{0xFFCBD5E1}; // Labels
inline const juce::Colour text_dim{0xFF64748B};    // Disabled, hints

// --- Cell age gradient (for grid visualization) ---
inline const juce::Colour cell_new{0xFF818CF8}; // Just born (indigo)
inline const juce::Colour cell_mid{0xFFA78BFA}; // Mid-age (purple)
inline const juce::Colour cell_old{0xFFF472B6}; // Ancient (pink)

// --- Status ---
inline const juce::Colour alive{0xFF22D3EE};   // Active/on (cyan)
inline const juce::Colour warning{0xFFFBBF24}; // Warning (amber)
inline const juce::Colour danger{0xFFEF4444};  // Error/clip (red)

// --- Knob ---
inline const juce::Colour knob_track{0xFF2A2A45}; // Inactive arc
inline const juce::Colour knob_fill{0xFF6366F1};  // Active arc

// --- Engine-specific visualization ---
// Brian's Brain
inline const juce::Colour bb_on{0xFF22D3EE};    // cyan (active)
inline const juce::Colour bb_dying{0xFFFBBF24}; // amber (dying)

// Float field engine base colors
inline const juce::Colour field_rd{0xFF3B82F6};    // blue (Reaction-Diffusion)
inline const juce::Colour field_lenia{0xFF10B981}; // emerald (Lenia)
inline const juce::Colour field_swarm{0xFFEC4899}; // pink (Particle Swarm)
inline const juce::Colour field_brown{0xFFF59E0B}; // amber (Brownian Field)

// --- Misc ---
inline const juce::Colour divider{0xFF2A2A45}; // Separator lines
inline const juce::Colour shadow{0x40000000};  // Drop shadow
} // namespace NebulaColours

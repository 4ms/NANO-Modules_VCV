#ifndef QUANTIZER_HPP
#define QUANTIZER_HPP

#include <cmath>

// Assuming a maximum of 12 notes per scale
#define MAX_NOTES_PER_SCALE 12

struct Scale {
    int notes[MAX_NOTES_PER_SCALE]; // Semitone intervals in the scale
    int size; // Number of notes in the scale
    enum scaleType{
        CHROMATIC,
        MAJOR,
        MINOR,
        DORIAN,
        PHRYGIAN,
        LYDIAN,
        MIXOLYDIAN,
        LOCRIAN,
        PENTAMAJ,
        PENTAMIN,
        HARM_MIN,
        MELODIC,
        BLUES,
        ARABIAN,
        ROMANIAN,
        BALINESE,
        HUNGAR_1,
        HUNGAR_2,
        ORIENTAL,
        RAGATOE,
        CHINESE,
        JAPAN_1,
        JAPAN_2,
        PERSIAN,
        DIMINISH,
        PHRYGDOM,
        BYZANTINE,
        SPAINPHRY,
        NEAPOLMIN,
        NEAPOLMAJ,
        UKRANDORI,
        SCALES,
    };
};

class Quantizer {
private:
    Scale scales[Scale::SCALES]; // Array of scales
    int currentScaleIndex; // Index of the currently selected scale
    bool quantizeActive; // Flag to control quantization bypass

public:
    Quantizer() {
        // Define scales
        scales[Scale::CHROMATIC] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 12};
        scales[Scale::MAJOR] = {{0, 2, 4, 5, 7, 9, 11}, 7};
        scales[Scale::MINOR] = {{0, 2, 3, 5, 7, 8, 10}, 7};
        scales[Scale::DORIAN] = {{0, 2, 3, 5, 7, 9, 10, 12, 14, 0, 0, 0}, 7};
        scales[Scale::PHRYGIAN] = {{0, 1, 3, 5, 7, 8, 10, 12, 13, 0, 0, 0}, 7};
        scales[Scale::LYDIAN] = {{0, 2, 4, 6, 7, 9, 11, 12, 14, 0, 0, 0}, 7};
        scales[Scale::MIXOLYDIAN] = {{0, 2, 4, 5, 7, 9, 10, 12, 14, 0, 0, 0}, 7};
        scales[Scale::LOCRIAN] = {{0, 1, 3, 5, 6, 8, 10, 12, 13, 0, 0, 0}, 7};
        scales[Scale::PENTAMAJ] = {{0, 2, 4, 7, 9, 12, 14, 16, 19, 0, 0, 0}, 5};
        scales[Scale::PENTAMIN] = {{0, 3, 5, 7, 10, 12, 15, 17, 19, 0, 0, 0}, 5};
        scales[Scale::HARM_MIN] = {{0, 2, 3, 5, 7, 8, 11, 12, 14, 0, 0, 0}, 7};
        scales[Scale::MELODIC] = {{0, 2, 3, 5, 7, 9, 11, 12, 14, 0, 0, 0}, 7};
        scales[Scale::BLUES] = {{0, 3, 5, 6, 7, 10, 12, 15, 17, 0, 0, 0}, 6};
        scales[Scale::ARABIAN] = {{0, 2, 4, 5, 6, 8, 10, 12, 14, 0, 0, 0}, 7};
        scales[Scale::ROMANIAN] = {{0, 2, 3, 6, 7, 9, 10, 12, 17, 0, 0, 0}, 7};
        scales[Scale::BALINESE] = {{0, 2, 3, 7, 8, 12, 14, 15, 19, 0, 0, 0}, 5};
        scales[Scale::HUNGAR_1] = {{0, 2, 3, 6, 7, 8, 11, 12, 14, 0, 0, 0}, 7};
        scales[Scale::HUNGAR_2] = {{0, 3, 4, 6, 7, 9, 10, 12, 15, 0, 0, 0}, 7};
        scales[Scale::ORIENTAL] = {{0, 1, 4, 5, 6, 8, 10, 12, 13, 0, 0, 0}, 7};
        scales[Scale::RAGATOE] = {{0, 1, 3, 6, 7, 8, 10, 12, 13, 0, 0, 0}, 7};
        scales[Scale::CHINESE] = {{0, 4, 6, 7, 11, 12, 16, 18, 19, 0, 0, 0}, 5};
        scales[Scale::JAPAN_1] = {{0, 1, 5, 7, 8, 12, 13, 17, 19, 0, 0, 0}, 5};
        scales[Scale::JAPAN_2] = {{0, 2, 5, 7, 8, 12, 14, 17, 19, 0, 0, 0}, 5};
        scales[Scale::PERSIAN] = {{0, 1, 4, 6, 8, 11, 12, 15, 17, 0, 0, 0}, 6};
        scales[Scale::DIMINISH] = {{0, 2, 3, 5, 6, 8, 9, 11, 12, 0, 0, 0}, 8};
        scales[Scale::PHRYGDOM] = {{0, 1, 4, 5, 7, 8, 10, 12, 13, 0, 0, 0}, 7};
        scales[Scale::BYZANTINE] = {{0, 1, 4, 5, 7, 8, 11, 12, 13, 0, 0, 0}, 7};
        scales[Scale::SPAINPHRY] = {{0, 1, 3, 4, 5, 7, 8, 10, 12, 0, 0, 0}, 8};
        scales[Scale::NEAPOLMIN] = {{0, 1, 3, 5, 7, 8, 11, 12, 13, 0, 0, 0}, 7};
        scales[Scale::NEAPOLMAJ] = {{0, 1, 3, 5, 7, 9, 11, 12, 13, 0, 0, 0}, 7};
        scales[Scale::UKRANDORI] = {{0, 2, 3, 6, 7, 9, 10, 12, 14, 0, 0, 0}, 7};

        // Default to Chromatic scale
        currentScaleIndex = 0;
    }

    // Method to enable or disable quantization
    void setQuantizeActive(bool active) {
        quantizeActive = active;
    }

    void setScale(int scaleIndex) {
        if (scaleIndex >= 0 && scaleIndex < Scale::SCALES) {
            currentScaleIndex = scaleIndex;
        }
        // Handle invalid index, if needed
    }

    float quantizeVoltage(float voltage) const {
        // If quantization is bypassed, return the input voltage directly
        if (!quantizeActive) {
            return voltage;
        }

        const Scale& currentScale = scales[currentScaleIndex];

        // Extract octave and semitone information from the voltage
        int totalSemitones = static_cast<int>(std::round(voltage * 12.0f));
        int octave = totalSemitones / 12; // Get the octave
        int semitoneInOctave = totalSemitones % 12; // Semitone within the octave

        // Find the closest note within the scale, ignoring the octave
        int closestNoteInScale = findClosestNoteInScale(currentScale, semitoneInOctave);

        // Calculate quantized voltage, adding the octave back
        float quantizedVoltage = (octave * 12 + closestNoteInScale) / 12.0f;
        return quantizedVoltage;
    }

private:
    int findClosestNoteInScale(const Scale& scale, int semitone) const {
        int closestNote = semitone;
        int smallestDifference = 12; // Maximum difference in semitones

        for (int i = 0; i < scale.size; ++i) {
            int note = scale.notes[i];
            int difference = std::abs(note - semitone);
            if (difference < smallestDifference) {
                closestNote = note;
                smallestDifference = difference;
            }
        }

        return closestNote;
    }
};

#endif // QUANTIZER_HPP

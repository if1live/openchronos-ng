/*!
 * \file buzzer.h
 * \brief Buzzer subsystem functions.
 * \details This file contains all the methods used for \
 * playing tones with buzzer.
 * The buzzer can play a number of different tones, represented as a
 * note array.
 * The buzzer output frequency can be calculated using:
 * \f[ \frac{T_{\mathrm{ACLK}}}{2*\mathrm{TA1CCR0}} \f]
 */
#ifndef BUZZER_H_
#define BUZZER_H_

/*!
 * Note type.
 * This is a type representing a note. It is composed by:
 *     
 *     MSB [duration:10][octave:2][pitch:4] LSB
 *
 * There are two "meta" notes:
 * - The note xxx0 represents no tone (a rest).
 * - The note xxxF represents the "stop note" marking the end of a note sequence.
 *
 * A stop note is **required**, since the length of the notes array is
 * not known.
 */
typedef uint16_t note;

/*!
 * \brief Initialize buzzer subsystem.
 */
void buzzer_init(void);

/*!
 * \brief Play a sequence of notes using the buzzer.
 * \param notes An array of notes to play.
 */
void buzzer_play(note *notes);

/**
 * Plays a short alert consisting of 1 beep.
 */
void buzzer_play_alert();

/**
 * Same as buzzer_play_alert, but 2 beeps in quick succession.
 * Can be called once a second.
 */ 
void buzzer_play_alert2x();

#endif /*BUZZER_H_*/

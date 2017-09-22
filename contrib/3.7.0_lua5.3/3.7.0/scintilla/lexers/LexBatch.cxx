// Scintilla source code edit control
/** @file LexBatch.cxx
 ** Lexer for batch files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif



static bool Is0To9(char ch) {
	return (ch >= '0') && (ch <= '9');
}

static bool IsAlphabetic(int ch) {
	return IsASCII(ch) && isalpha(ch);
}

static inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
static bool IsBOperator(char ch) {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') ||
	       (ch == '|') || (ch == '?') || (ch == '*');
}

// Tests for BATCH Separators
static bool IsBSeparator(char ch) {
	return (ch == '\\') || (ch == '.') || (ch == ';') ||
	       (ch == '\"') || (ch == '\'') || (ch == '/');
}


struct sBatchState {
	int status;
	int offset;
	int wbLen;
	int wbPos;
	int cmdLoc;
	int startLine;
} pBatchState;

/// @info colour a keyword prefixed by a separator.
/// @return offset and cmdloc

static int ColourSeparatorKw(
	char *wordBuffer,
	sBatchState *pBState,
	WordList *keywords2,
	Accessor &styler) {


	if (IsBSeparator(wordBuffer[0])) {
		// then Check for External Command / Program following the sep.
		if ((pBState->cmdLoc == pBState->offset - pBState->wbLen) && ((wordBuffer[0] == ':') || (wordBuffer[0] == '\\') || (wordBuffer[0] == '.'))) {
			// Yo. Reset Offset to re-process remainder of word
			pBState->status =1;
			pBState->offset -= (pBState->wbLen - 1);

			// Colorize External Command / Program
			if (!keywords2) {
				styler.ColourTo(pBState->startLine + pBState->offset - 1, SCE_BAT_COMMAND);
			} else if (keywords2->InList(wordBuffer)) {
				styler.ColourTo(pBState->startLine + pBState->offset - 1, SCE_BAT_COMMAND);
			} else {
				styler.ColourTo(pBState->startLine + pBState->offset - 1, SCE_BAT_DEFAULT);
			}

			// Reset External Command / Program Location
			pBState->cmdLoc = pBState->offset;
		} else {
			// No ExtCmd found - Reset Offset to re-process remainder of word
			pBState->offset -= (pBState->wbLen - 1);
			pBState->status =0;
			// Colorize Default Text
			styler.ColourTo(pBState->startLine + pBState->offset - 1, SCE_BAT_DEFAULT);
		}
	}

	return (pBState->status);
}


static void ColouriseBatchLine(
	char *lineBuffer,
	Sci_PositionU lengthLine,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
	WordList *keywordlists[],
	Accessor &styler) {

	Sci_PositionU offset = 0;	// Line Buffer Offset
	Sci_PositionU cmdLoc;		// External Command / Program Location
	char wordBuffer[81];		// Word Buffer - large to catch long paths
	Sci_PositionU wbLen;		// Word Buffer Length
	Sci_PositionU wbPos;		// Word Buffer Offset - also Special Keyword Buffer Length
	WordList &keywords = *keywordlists[0];      // Internal Commands
	WordList &keywords2 = *keywordlists[1];     // External Commands (optional)

	// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
	//   Toggling Regular Keyword Checking off improves readability
	// Other Regular Keywords and External Commands / Programs might also benefit from toggling
	//   Need a more robust algorithm to properly toggle Regular Keyword Checking
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	// Special Keywords are those that allow certain characters without whitespace after the command
	// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
	// Special Keyword Buffer used to determine if the first n characters is a Keyword
	char sKeywordBuffer[10];	// Special Keyword Buffer
	bool sKeywordFound;		// Exit Special Keyword for-loop if found

	// Skip initial spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}
	// Colorize Default Text
	styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);


	// Set External Command / Program Location to the position directly after any space.
	cmdLoc = offset;


	// Check for Fake Label (Comment) or Real Label - return if found
	if (lineBuffer[offset] == ':') {
		if (lineBuffer[offset + 1] == ':') {
			// Colorize Fake Label (Comment) - :: is similar to REM, see http://content.techweb.com/winmag/columns/explorer/2000/21.htm
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
		} else {
			// Colorize Real Label
			styler.ColourTo(endPos, SCE_BAT_LABEL);
		}
		return;
		// Check for Drive Change (Drive Change is internal command) - return if found
	} else if ((IsAlphabetic(lineBuffer[offset])) &&
			(lineBuffer[offset + 1] == ':') &&
			((isspacechar(lineBuffer[offset + 2])) ||
			 (((lineBuffer[offset + 2] == '\\')) &&
			  (isspacechar(lineBuffer[offset + 3]))))) {
		// Colorize Regular Keyword
		styler.ColourTo(endPos, SCE_BAT_WORD);
		return;
	}


	// Check for Hide Command (@ECHO OFF/ON)
	if (lineBuffer[offset] == '@') {
		styler.ColourTo(startLine + offset, SCE_BAT_HIDE);
		offset++;
	}
	// Skip next spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}

	wbLen = 0;
	wbPos = 0;

	// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
	while (offset < lengthLine) {
		if (offset > startLine) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
		}
		// Copy word from Line Buffer into Word Buffer
		wbLen = 0;
		for (; offset < lengthLine && wbLen < 80 &&
				!isspacechar(lineBuffer[offset]); wbLen++, offset++) {
			wordBuffer[wbLen] = static_cast<char>(tolower(lineBuffer[offset]));
		}
		wordBuffer[wbLen] = '\0';
		wbPos = 0;

		// Check for Comment - return if found
		if (CompareCaseInsensitive(wordBuffer, "rem") == 0) {
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
			return;
		}


		// So, we have checked that the current word is not a comment.
		// now Check for a Separator at position zero.

/// refactorisation 1 lets see if we are on the right track....

		pBatchState.startLine=startLine;
		pBatchState.offset=offset;
		pBatchState.wbLen=wbLen;
		pBatchState.wbPos=wbPos;
		pBatchState.cmdLoc=cmdLoc;
		pBatchState.status=ColourSeparatorKw(wordBuffer, &pBatchState, &keywords2, styler);
		offset=pBatchState.offset;
		wbLen=pBatchState.wbLen;
		wbPos=pBatchState.wbPos;
		cmdLoc=pBatchState.cmdLoc;
		startLine=pBatchState.startLine;


/// refactorisation 2

		if (!IsBSeparator(wordBuffer[0])) {
			// Check for Regular Keyword in list
			if ((keywords.InList(wordBuffer)) && (continueProcessing)) {
				// ECHO, GOTO, PROMPT and SET require no further Regular Keyword Checking
				if ((CompareCaseInsensitive(wordBuffer, "echo") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "goto") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "prompt") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "set") == 0)) {
					continueProcessing = false;
				}
				// Identify External Command / Program Location for ERRORLEVEL, and EXIST
				if ((CompareCaseInsensitive(wordBuffer, "errorlevel") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "exist") == 0)) {
					// Reset External Command / Program Location
					cmdLoc = offset;
					// Skip next spaces
					while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
					// Skip comparison
					while ((cmdLoc < lengthLine) &&
							(!isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
					// Skip next spaces
					while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
					// Identify External Command / Program Location for CALL, DO, LOADHIGH and LH
				} else if ((CompareCaseInsensitive(wordBuffer, "call") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "do") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "loadhigh") == 0) ||
						(CompareCaseInsensitive(wordBuffer, "lh") == 0)) {
					// Reset External Command / Program Location
					cmdLoc = offset;
					// Skip next spaces
					while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
				}
				// Colorize Regular keyword
				styler.ColourTo(startLine + offset - 1, SCE_BAT_WORD);
				// No need to Reset Offset
			}
		}

/// refactorisation 3

		// Check for Special Keyword in list, External Command / Program, or Default Text
		if (!IsBSeparator(wordBuffer[0])) {
			if ((wordBuffer[0] != '%') &&
					(wordBuffer[0] != '!') &&
					(!IsBOperator(wordBuffer[0])) &&
					(continueProcessing)) {
				// Check for Special Keyword
				//     Affected Commands are in Length range 2-6
				//     Good that ERRORLEVEL, EXIST, CALL, DO, LOADHIGH, and LH are unaffected
				sKeywordFound = false;
				for (Sci_PositionU keywordLength = 2; keywordLength < wbLen && keywordLength < 7 && !sKeywordFound; keywordLength++) {
					wbPos = 0;
					// Copy Keyword Length from Word Buffer into Special Keyword Buffer
					for (; wbPos < keywordLength; wbPos++) {
						sKeywordBuffer[wbPos] = static_cast<char>(wordBuffer[wbPos]);
					}
					sKeywordBuffer[wbPos] = '\0';
					// Check for Special Keyword in list
					if ((keywords.InList(sKeywordBuffer)) &&
							((IsBOperator(wordBuffer[wbPos])) ||
							 (IsBSeparator(wordBuffer[wbPos])))) {
						sKeywordFound = true;
						// ECHO requires no further Regular Keyword Checking
						if (CompareCaseInsensitive(sKeywordBuffer, "echo") == 0) {
							continueProcessing = false;
						}
						// Colorize Special Keyword as Regular Keyword
						styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_WORD);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - wbPos);
					}
				}
				// Check for External Command / Program or Default Text
				if (!sKeywordFound) {
					wbPos = 0;
					// Check for External Command / Program
					if (cmdLoc == offset - wbLen) {
						// Read up to %, Operator or Separator
						while ((wbPos < wbLen) &&
								(wordBuffer[wbPos] != '%') &&
								(wordBuffer[wbPos] != '!') &&
								(!IsBOperator(wordBuffer[wbPos])) &&
								(!IsBSeparator(wordBuffer[wbPos]))) {
							wbPos++;
						}
						// Reset External Command / Program Location
						cmdLoc = offset - (wbLen - wbPos);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - wbPos);
						// CHOICE requires no further Regular Keyword Checking
						if (CompareCaseInsensitive(wordBuffer, "choice") == 0) {
							continueProcessing = false;
						}
						// Check for START (and its switches) - What follows is External Command \ Program
						if (CompareCaseInsensitive(wordBuffer, "start") == 0) {
							// Reset External Command / Program Location
							cmdLoc = offset;
							// Skip next spaces
							while ((cmdLoc < lengthLine) &&
									(isspacechar(lineBuffer[cmdLoc]))) {
								cmdLoc++;
							}
							// Reset External Command / Program Location if command switch detected
							if (lineBuffer[cmdLoc] == '/' || lineBuffer[cmdLoc] == '-') {
								// Skip command switch
								while ((cmdLoc < lengthLine) &&
										(!isspacechar(lineBuffer[cmdLoc]))) {
									cmdLoc++;
								}
								styler.ColourTo(startLine + offset - 1, SCE_BAT_OPERATOR);
								// Skip next spaces
								while ((cmdLoc < lengthLine) &&
										(isspacechar(lineBuffer[cmdLoc]))) {
									cmdLoc++;
								}
							}
						}
						// Colorize External Command / Program
						if (!keywords2) {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
						} else if (keywords2.InList(wordBuffer)) {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
						} else {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
						}
						// No need to Reset Offset
						// Check for Default Text
					} else {
						// Read up to %, Operator or Separator
						while ((wbPos < wbLen) &&
								(wordBuffer[wbPos] != '%') &&
								(wordBuffer[wbPos] != '!') &&
								(!IsBOperator(wordBuffer[wbPos])) &&
								(!IsBSeparator(wordBuffer[wbPos]))) {
							wbPos++;
						}
						// Colorize Default Text
						styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - wbPos);
					}
				}
			}

/// refactorisation 4

			// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
			if (!IsBSeparator(wordBuffer[0])) {
				if (wordBuffer[0] == '%') {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbLen, SCE_BAT_DEFAULT);
					wbPos++;
					// Search to end of word for second % (can be a long path)
					while ((wbPos < wbLen) &&
							(wordBuffer[wbPos] != '%') &&
							(!IsBOperator(wordBuffer[wbPos])) &&
							(!IsBSeparator(wordBuffer[wbPos]))) {
						wbPos++;
					}
					// Check for Argument (%n) or (%*)
					if (((Is0To9(wordBuffer[1])) || (wordBuffer[1] == '*')) &&
							(wordBuffer[wbPos] != '%')) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbLen) {
							cmdLoc = offset - (wbLen - 2);
						}
						// Colorize Argument
						styler.ColourTo(startLine + offset - 1 - (wbLen - 2), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - 2);
						// Check for Expanded Argument (%~...) / Variable (%%~...)
					} else if (((wbLen > 1) && (wordBuffer[1] == '~')) ||
							((wbLen > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] == '~'))) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbLen) {
							cmdLoc = offset - (wbLen - wbPos);
						}
						// Colorize Expanded Argument / Variable
						styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - wbPos);
						// Check for Environment Variable (%x...%)
					} else if ((wordBuffer[1] != '%') &&
							(wordBuffer[wbPos] == '%')) {
						wbPos++;
						// Check for External Command / Program
						if (cmdLoc == offset - wbLen) {
							cmdLoc = offset - (wbLen - wbPos);
						}
						// Colorize Environment Variable
						styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - wbPos);
						// Check for Local Variable (%%a)
					} else if (
						(wbLen > 2) &&
						(wordBuffer[1] == '%') &&
						(wordBuffer[2] != '%') &&
						(!IsBOperator(wordBuffer[2])) &&
						(!IsBSeparator(wordBuffer[2]))) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbLen) {
							cmdLoc = offset - (wbLen - 3);
						}
						// Colorize Local Variable
						styler.ColourTo(startLine + offset - 1 - (wbLen - 3), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbLen - 3);
					}
				}
			}

/// refactorisation 5

			if (!IsBSeparator(wordBuffer[0])) {
				// Check for Environment Variable (!x...!)
			} else if (wordBuffer[0] == '!') {
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1 - wbLen, SCE_BAT_DEFAULT);
				wbPos++;
				// Search to end of word for second ! (can be a long path)
				while ((wbPos < wbLen) &&
						(wordBuffer[wbPos] != '!') &&
						(!IsBOperator(wordBuffer[wbPos])) &&
						(!IsBSeparator(wordBuffer[wbPos]))) {
					wbPos++;
				}
				if (wordBuffer[wbPos] == '!') {
					wbPos++;
					// Check for External Command / Program
					if (cmdLoc == offset - wbLen) {
						cmdLoc = offset - (wbLen - wbPos);
					}
					// Colorize Environment Variable
					styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_IDENTIFIER);
					// Reset Offset to re-process remainder of word
					offset -= (wbLen - wbPos);
				}

				// Check for Operator
			} else if (IsBOperator(wordBuffer[0])) {
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1 - wbLen, SCE_BAT_DEFAULT);
				// Check for Comparison Operator
				if ((wordBuffer[0] == '=') && (wordBuffer[1] == '=')) {
					// Identify External Command / Program Location for IF
					cmdLoc = offset;
					// Skip next spaces
					while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
					// Colorize Comparison Operator
					styler.ColourTo(startLine + offset - 1 - (wbLen - 2), SCE_BAT_OPERATOR);
					// Reset Offset to re-process remainder of word
					offset -= (wbLen - 2);
					// Check for Pipe Operator
				} else if (wordBuffer[0] == '|') {
					// Reset External Command / Program Location
					cmdLoc = offset - wbLen + 1;
					// Skip next spaces
					while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
						cmdLoc++;
					}
					// Colorize Pipe Operator
					styler.ColourTo(startLine + offset - 1 - (wbLen - 1), SCE_BAT_OPERATOR);
					// Reset Offset to re-process remainder of word
					offset -= (wbLen - 1);
					// Check for Other Operator
				} else {
					// Check for > Operator
					if (wordBuffer[0] == '>') {
						// Turn Keyword and External Command / Program checking back on
						continueProcessing = true;
					}
					// Colorize Other Operator
					styler.ColourTo(startLine + offset - 1 - (wbLen - 1), SCE_BAT_OPERATOR);
					// Reset Offset to re-process remainder of word
					offset -= (wbLen - 1);
				}
				// Check for Default Text
			} else {
				// Read up to %, Operator or Separator
				while ((wbPos < wbLen) &&
						(wordBuffer[wbPos] != '%') &&
						(wordBuffer[wbPos] != '!') &&
						(!IsBOperator(wordBuffer[wbPos])) &&
						(!IsBSeparator(wordBuffer[wbPos]))) {
					wbPos++;
				}
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1 - (wbLen - wbPos), SCE_BAT_DEFAULT);
				// Reset Offset to re-process remainder of word
				offset -= (wbLen - wbPos);
			}
		}

		// Skip next spaces - nothing happens if Offset was Reset
		while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
			offset++;
		}
	}


	// Colorize Default Text for remainder of line - currently not lexed
	styler.ColourTo(endPos, SCE_BAT_DEFAULT);
}

static void ColouriseBatchDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int /*initStyle*/,
	WordList *keywordlists[],
	Accessor &styler) {

	char lineBuffer[1024];

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	Sci_PositionU startLine = startPos;
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseBatchLine(lineBuffer, linePos, startLine, i, keywordlists, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		lineBuffer[linePos] = '\0';
		ColouriseBatchLine(lineBuffer, linePos, startLine, startPos + length - 1,
				   keywordlists, styler);
	}
}

static const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	0
};

LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", 0, batchWordListDesc);

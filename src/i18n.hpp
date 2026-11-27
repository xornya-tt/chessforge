#pragma once

#include <string>

namespace chessforge {

// Interface languages. The built-in 5x7 font covers Latin (without accents) and Cyrillic.
constexpr int LanguageCount = 8;

// Keep the order identical to the rows of the table in i18n.cpp (one row per string).
enum class Str : int {
    StateWhiteMove,
    StateBlackMove,
    StateWhiteWins,
    StateBlackWins,
    StateStalemate,
    StateFifty,
    InCheck,
    EngineStockfish,
    EngineBuiltin,
    YouWhite,
    YouBlack,
    TwoPlayers,
    View3d,
    View2d,
    HintMove,
    HintUndo,
    HintView,
    HintSettings,
    HintNew,
    HintNewShort,
    HintCamera,
    HintHelp,
    NRestored,
    NNew,
    NSelectDest,
    NPieceChanged,
    NIllegal,
    NSfThink,
    NBiThink,
    NSfMoved,
    NBiMoved,
    NUndone,
    NControlsReset,
    NBound,
    NSwapped,
    NCancelled,
    Board3dLabel,
    Settings,
    SLanguage,
    SMode,
    STheme,
    STexture,
    SElo,
    SSide,
    SFlip,
    SCamera,
    SPieces,
    SLight,
    SDark,
    SAccent,
    SBack,
    SControls,
    VVsComputer,
    VFixed,
    VAutoFlip,
    VWhite,
    VBlack,
    VBuiltin,
    VOak,
    VViolet,
    VForest,
    VMono,
    VCustom,
    VFlat,
    VWood,
    VMarble,
    VCarbon,
    VFelt,
    FChange,
    FConfirm,
    FBack,
    FPieceFolder,
    ELight,
    EDark,
    EAccent,
    EBackground,
    EEdit,
    ERed,
    EGreen,
    EBlue,
    EHelp,
    ESave,
    HelpClose,
    HelpCursor,
    AUp,
    ADown,
    ALeft,
    ARight,
    AConfirm,
    ABack,
    AView,
    ACamLeft,
    ACamRight,
    AQuit,
    CPress,
    CWait,
    CReset,
    CNone,
    CFoot1,
    CFoot2,
    Count
};

[[nodiscard]] const char* languageCode(int index);   // "en", "ru", ...
[[nodiscard]] const char* languageName(int index);   // shown in the language picker
[[nodiscard]] int languageFromCode(const std::string& code);  // -1 if unknown

void setLanguage(int index);
[[nodiscard]] int language();

// Text of a string in the current language; falls back to English.
[[nodiscard]] const char* tr(Str key);
[[nodiscard]] const char* trIn(Str key, int languageIndex);

}  // namespace chessforge

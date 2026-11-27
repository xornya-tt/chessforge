#include "i18n.hpp"

#include <array>
#include <cstddef>

namespace chessforge {
namespace {

struct Language {
    const char* code;
    const char* name;
};

constexpr std::array<Language, 8> Languages{{
    {"en", "ENGLISH"},
    {"ru", "РУССКИЙ"},
    {"uk", "УКРАЇНСЬКА"},
    {"de", "DEUTSCH"},
    {"es", "ESPANOL"},
    {"fr", "FRANCAIS"},
    {"it", "ITALIANO"},
    {"pt", "PORTUGUES"},
}};

// One row per string, columns in the order of `Languages`:
// en  ru  uk  de  es  fr  it  pt
// To fix or add a translation edit the row; keep the font in mind (Latin without accents, Cyrillic).
using Row = std::array<const char*, LanguageCount>;
constexpr std::array<Row, static_cast<std::size_t>(Str::Count)> Table{{
    {{"WHITE TO MOVE", "ХОД БЕЛЫХ", "ХІД БІЛИХ", "WEISS AM ZUG", "JUEGAN BLANCAS", "AUX BLANCS", "MUOVE IL BIANCO", "VEZ DAS BRANCAS"}},  // StateWhiteMove
    {{"BLACK TO MOVE", "ХОД ЧЁРНЫХ", "ХІД ЧОРНИХ", "SCHWARZ AM ZUG", "JUEGAN NEGRAS", "AUX NOIRS", "MUOVE IL NERO", "VEZ DAS PRETAS"}},  // StateBlackMove
    {{"CHECKMATE WHITE WINS", "МАТ БЕЛЫЕ ВЫИГРАЛИ", "МАТ БІЛІ ПЕРЕМОГЛИ", "MATT WEISS GEWINNT", "MATE GANAN BLANCAS", "MAT BLANCS GAGNENT", "SCACCOMATTO VINCE BIANCO", "MATE BRANCAS VENCEM"}},  // StateWhiteWins
    {{"CHECKMATE BLACK WINS", "МАТ ЧЁРНЫЕ ВЫИГРАЛИ", "МАТ ЧОРНІ ПЕРЕМОГЛИ", "MATT SCHWARZ GEWINNT", "MATE GANAN NEGRAS", "MAT NOIRS GAGNENT", "SCACCOMATTO VINCE NERO", "MATE PRETAS VENCEM"}},  // StateBlackWins
    {{"DRAW STALEMATE", "НИЧЬЯ ПАТ", "НІЧИЯ ПАТ", "REMIS PATT", "TABLAS AHOGADO", "NULLE PAT", "PATTA", "EMPATE AFOGAMENTO"}},  // StateStalemate
    {{"DRAW FIFTY MOVE", "НИЧЬЯ 50 ХОДОВ", "НІЧИЯ 50 ХОДІВ", "REMIS 50 ZUEGE", "TABLAS 50 JUGADAS", "NULLE 50 COUPS", "PATTA 50 MOSSE", "EMPATE 50 LANCES"}},  // StateFifty
    {{"CHECK", "ШАХ", "ШАХ", "SCHACH", "JAQUE", "ECHEC", "SCACCO", "XEQUE"}},  // InCheck
    {{"ENGINE STOCKFISH", "ДВИЖОК STOCKFISH", "РУШІЙ STOCKFISH", "MOTOR STOCKFISH", "MOTOR STOCKFISH", "MOTEUR STOCKFISH", "MOTORE STOCKFISH", "MOTOR STOCKFISH"}},  // EngineStockfish
    {{"ENGINE BUILTIN", "ДВИЖОК ВСТРОЕННЫЙ", "РУШІЙ ВБУДОВАНИЙ", "MOTOR EINGEBAUT", "MOTOR INTEGRADO", "MOTEUR INTEGRE", "MOTORE INTEGRATO", "MOTOR INTEGRADO"}},  // EngineBuiltin
    {{"YOU WHITE", "ВЫ БЕЛЫМИ", "ВИ БІЛИМИ", "SIE WEISS", "TU BLANCAS", "VOUS BLANCS", "TU BIANCO", "VOCE BRANCAS"}},  // YouWhite
    {{"YOU BLACK", "ВЫ ЧЁРНЫМИ", "ВИ ЧОРНИМИ", "SIE SCHWARZ", "TU NEGRAS", "VOUS NOIRS", "TU NERO", "VOCE PRETAS"}},  // YouBlack
    {{"TWO PLAYERS", "ДВА ИГРОКА", "ДВА ГРАВЦІ", "ZWEI SPIELER", "DOS JUGADORES", "DEUX JOUEURS", "DUE GIOCATORI", "DOIS JOGADORES"}},  // TwoPlayers
    {{"VIEW 3D", "ВИД 3D", "ВИГЛЯД 3D", "ANSICHT 3D", "VISTA 3D", "VUE 3D", "VISTA 3D", "VISTA 3D"}},  // View3d
    {{"VIEW 2D", "ВИД 2D", "ВИГЛЯД 2D", "ANSICHT 2D", "VISTA 2D", "VUE 2D", "VISTA 2D", "VISTA 2D"}},  // View2d
    {{"MOVE", "ХОД", "ХІД", "ZIEHEN", "MOVER", "JOUER", "MUOVI", "MOVER"}},  // HintMove
    {{"UNDO", "ОТМЕНА", "СКАСУВАТИ", "ZURUECK", "DESHACER", "ANNULER", "ANNULLA", "DESFAZER"}},  // HintUndo
    {{"2D 3D", "2D 3D", "2D 3D", "2D 3D", "2D 3D", "2D 3D", "2D 3D", "2D 3D"}},  // HintView
    {{"SETTINGS", "НАСТРОЙКИ", "НАЛАШТУВАННЯ", "OPTIONEN", "AJUSTES", "REGLAGES", "OPZIONI", "AJUSTES"}},  // HintSettings
    {{"NEW GAME", "НОВАЯ ИГРА", "НОВА ГРА", "NEUES SPIEL", "NUEVO JUEGO", "NOUVELLE PARTIE", "NUOVA PARTITA", "NOVO JOGO"}},  // HintNew
    {{"NEW", "НОВАЯ", "НОВА", "NEU", "NUEVA", "NOUV", "NUOVA", "NOVO"}},  // HintNewShort
    {{"CAMERA", "КАМЕРА", "КАМЕРА", "KAMERA", "CAMARA", "CAMERA", "CAMERA", "CAMERA"}},  // HintCamera
    {{"HELP", "ПОМОЩЬ", "ДОВІДКА", "HILFE", "AYUDA", "AIDE", "AIUTO", "AJUDA"}},  // HintHelp
    {{"GAME RESTORED", "ПАРТИЯ ВОССТАНОВЛЕНА", "ГРУ ВІДНОВЛЕНО", "SPIEL GELADEN", "PARTIDA RESTAURADA", "PARTIE RESTAUREE", "PARTITA RIPRISTINATA", "JOGO RESTAURADO"}},  // NRestored
    {{"NEW GAME", "НОВАЯ ПАРТИЯ", "НОВА ПАРТІЯ", "NEUES SPIEL", "NUEVA PARTIDA", "NOUVELLE PARTIE", "NUOVA PARTITA", "NOVO JOGO"}},  // NNew
    {{"SELECT DESTINATION", "ВЫБЕРИТЕ ПОЛЕ", "ОБЕРІТЬ ПОЛЕ", "ZIELFELD WAEHLEN", "ELIGE CASILLA", "CHOISIR LA CASE", "SCEGLI LA CASA", "ESCOLHA A CASA"}},  // NSelectDest
    {{"PIECE CHANGED", "ФИГУРА ИЗМЕНЕНА", "ФІГУРУ ЗМІНЕНО", "FIGUR GEWECHSELT", "PIEZA CAMBIADA", "PIECE CHANGEE", "PEZZO CAMBIATO", "PECA TROCADA"}},  // NPieceChanged
    {{"ILLEGAL MOVE", "НЕЛЬЗЯ ТАК ХОДИТЬ", "НЕМОЖЛИВИЙ ХІД", "UNGUELTIGER ZUG", "MOVIMIENTO ILEGAL", "COUP ILLEGAL", "MOSSA ILLEGALE", "LANCE ILEGAL"}},  // NIllegal
    {{"STOCKFISH THINKING", "STOCKFISH ДУМАЕТ", "STOCKFISH ДУМАЄ", "STOCKFISH DENKT", "STOCKFISH PIENSA", "STOCKFISH REFLECHIT", "STOCKFISH PENSA", "STOCKFISH PENSA"}},  // NSfThink
    {{"BUILTIN BOT THINKING", "БОТ ДУМАЕТ", "БОТ ДУМАЄ", "BOT DENKT", "EL BOT PIENSA", "LE BOT REFLECHIT", "IL BOT PENSA", "O BOT PENSA"}},  // NBiThink
    {{"STOCKFISH MOVED", "STOCKFISH СХОДИЛ", "STOCKFISH ЗРОБИВ ХІД", "STOCKFISH ZOG", "STOCKFISH MOVIO", "STOCKFISH A JOUE", "STOCKFISH HA MOSSO", "STOCKFISH JOGOU"}},  // NSfMoved
    {{"BUILTIN BOT MOVED", "БОТ СХОДИЛ", "БОТ ЗРОБИВ ХІД", "BOT ZOG", "EL BOT MOVIO", "LE BOT A JOUE", "IL BOT HA MOSSO", "O BOT JOGOU"}},  // NBiMoved
    {{"TURN UNDONE", "ХОД ОТМЕНЁН", "ХІД СКАСОВАНО", "ZUG ZURUECK", "JUGADA DESHECHA", "COUP ANNULE", "MOSSA ANNULLATA", "LANCE DESFEITO"}},  // NUndone
    {{"CONTROLS RESET", "УПРАВЛЕНИЕ СБРОШЕНО", "КЕРУВАННЯ СКИНУТО", "STEUERUNG ZURUECK", "CONTROLES RESTABLECIDOS", "COMMANDES REINITIALISEES", "COMANDI RIPRISTINATI", "CONTROLES REDEFINIDOS"}},  // NControlsReset
    {{"BUTTON ASSIGNED", "КНОПКА НАЗНАЧЕНА", "КНОПКУ ПРИЗНАЧЕНО", "TASTE ZUGEWIESEN", "BOTON ASIGNADO", "BOUTON ASSIGNE", "TASTO ASSEGNATO", "BOTAO ATRIBUIDO"}},  // NBound
    {{"BUTTONS SWAPPED", "КНОПКИ ПОМЕНЯНЫ МЕСТАМИ", "КНОПКИ ПОМІНЯНО МІСЦЯМИ", "TASTEN GETAUSCHT", "BOTONES INTERCAMBIADOS", "BOUTONS ECHANGES", "TASTI SCAMBIATI", "BOTOES TROCADOS"}},  // NSwapped
    {{"CANCELLED", "ОТМЕНЕНО", "СКАСОВАНО", "ABGEBROCHEN", "CANCELADO", "ANNULE", "ANNULLATO", "CANCELADO"}},  // NCancelled
    {{"3D BOARD", "3D ДОСКА", "3D ДОШКА", "3D BRETT", "TABLERO 3D", "PLATEAU 3D", "SCACCHIERA 3D", "TABULEIRO 3D"}},  // Board3dLabel
    {{"SETTINGS", "НАСТРОЙКИ", "НАЛАШТУВАННЯ", "OPTIONEN", "AJUSTES", "REGLAGES", "OPZIONI", "AJUSTES"}},  // Settings
    {{"LANGUAGE", "ЯЗЫК", "МОВА", "SPRACHE", "IDIOMA", "LANGUE", "LINGUA", "IDIOMA"}},  // SLanguage
    {{"GAME MODE", "РЕЖИМ ИГРЫ", "РЕЖИМ ГРИ", "SPIELMODUS", "MODO DE JUEGO", "MODE DE JEU", "MODALITA", "MODO DE JOGO"}},  // SMode
    {{"THEME", "ТЕМА", "ТЕМА", "FARBSCHEMA", "TEMA", "THEME", "TEMA", "TEMA"}},  // STheme
    {{"TEXTURE", "ТЕКСТУРА", "ТЕКСТУРА", "OBERFLAECHE", "TEXTURA", "TEXTURE", "TEXTURE", "TEXTURA"}},  // STexture
    {{"BOT ELO", "УРОВЕНЬ БОТА", "РІВЕНЬ БОТА", "BOT ELO", "ELO DEL BOT", "ELO DU BOT", "ELO DEL BOT", "ELO DO BOT"}},  // SElo
    {{"YOUR SIDE", "ВАШ ЦВЕТ", "ВАШ КОЛІР", "IHRE FARBE", "TU COLOR", "VOTRE COULEUR", "IL TUO COLORE", "SUA COR"}},  // SSide
    {{"BOARD TURN", "ПОВОРОТ ДОСКИ", "ПОВОРОТ ДОШКИ", "BRETT DREHEN", "GIRO DEL TABLERO", "ROTATION", "ROTAZIONE", "GIRO DO TABULEIRO"}},  // SFlip
    {{"CAMERA", "КАМЕРА", "КАМЕРА", "KAMERA", "CAMARA", "CAMERA", "CAMERA", "CAMERA"}},  // SCamera
    {{"PIECES", "ФИГУРЫ", "ФІГУРИ", "FIGUREN", "PIEZAS", "PIECES", "PEZZI", "PECAS"}},  // SPieces
    {{"LIGHT RGB", "СВЕТЛЫЕ RGB", "СВІТЛІ RGB", "HELL RGB", "CLARAS RGB", "CLAIRES RGB", "CHIARE RGB", "CLARAS RGB"}},  // SLight
    {{"DARK RGB", "ТЁМНЫЕ RGB", "ТЕМНІ RGB", "DUNKEL RGB", "OSCURAS RGB", "FONCEES RGB", "SCURE RGB", "ESCURAS RGB"}},  // SDark
    {{"ACCENT RGB", "АКЦЕНТ RGB", "АКЦЕНТ RGB", "AKZENT RGB", "ACENTO RGB", "ACCENT RGB", "ACCENTO RGB", "DESTAQUE RGB"}},  // SAccent
    {{"BACK RGB", "ФОН RGB", "ФОН RGB", "HINTERGRUND RGB", "FONDO RGB", "FOND RGB", "SFONDO RGB", "FUNDO RGB"}},  // SBack
    {{"CONTROLS", "УПРАВЛЕНИЕ", "КЕРУВАННЯ", "STEUERUNG", "CONTROLES", "COMMANDES", "COMANDI", "CONTROLES"}},  // SControls
    {{"VS COMPUTER", "ПРОТИВ БОТА", "ПРОТИ БОТА", "GEGEN COMPUTER", "VS ORDENADOR", "CONTRE ORDI", "CONTRO PC", "VS COMPUTADOR"}},  // VVsComputer
    {{"FIXED", "НЕПОДВИЖНО", "НЕРУХОМО", "FEST", "FIJO", "FIXE", "FISSO", "FIXO"}},  // VFixed
    {{"AUTO FLIP", "АВТО РАЗВОРОТ", "АВТО РОЗВОРОТ", "AUTO DREHEN", "AUTO GIRO", "AUTO", "AUTO", "AUTO GIRO"}},  // VAutoFlip
    {{"WHITE", "БЕЛЫЕ", "БІЛІ", "WEISS", "BLANCAS", "BLANCS", "BIANCO", "BRANCAS"}},  // VWhite
    {{"BLACK", "ЧЁРНЫЕ", "ЧОРНІ", "SCHWARZ", "NEGRAS", "NOIRS", "NERO", "PRETAS"}},  // VBlack
    {{"BUILT-IN", "ВСТРОЕННЫЕ", "ВБУДОВАНІ", "EINGEBAUT", "INTEGRADAS", "INTEGREES", "INTEGRATI", "INTEGRADAS"}},  // VBuiltin
    {{"OAK", "ДУБ", "ДУБ", "EICHE", "ROBLE", "CHENE", "QUERCIA", "CARVALHO"}},  // VOak
    {{"VIOLET", "ФИОЛЕТ", "ФІОЛЕТ", "VIOLETT", "VIOLETA", "VIOLET", "VIOLA", "VIOLETA"}},  // VViolet
    {{"FOREST", "ЛЕС", "ЛІС", "WALD", "BOSQUE", "FORET", "FORESTA", "FLORESTA"}},  // VForest
    {{"MONO", "МОНО", "МОНО", "MONO", "MONO", "MONO", "MONO", "MONO"}},  // VMono
    {{"CUSTOM", "СВОЯ", "СВОЯ", "EIGENE", "PROPIA", "PERSO", "PERSONALE", "PERSONAL"}},  // VCustom
    {{"FLAT", "ГЛАДКАЯ", "ГЛАДКА", "GLATT", "LISA", "UNIE", "LISCIA", "LISA"}},  // VFlat
    {{"WOOD", "ДЕРЕВО", "ДЕРЕВО", "HOLZ", "MADERA", "BOIS", "LEGNO", "MADEIRA"}},  // VWood
    {{"MARBLE", "МРАМОР", "МАРМУР", "MARMOR", "MARMOL", "MARBRE", "MARMO", "MARMORE"}},  // VMarble
    {{"CARBON", "КАРБОН", "КАРБОН", "KARBON", "CARBONO", "CARBONE", "CARBONIO", "CARBONO"}},  // VCarbon
    {{"FELT", "ФЕТР", "ФЕТР", "FILZ", "FIELTRO", "FEUTRE", "FELTRO", "FELTRO"}},  // VFelt
    {{"LEFT RIGHT: CHANGE VALUE", "ВЛЕВО ВПРАВО: ИЗМЕНИТЬ ЗНАЧЕНИЕ", "ВЛІВО ВПРАВО: ЗМІНИТИ ЗНАЧЕННЯ", "LINKS RECHTS: WERT AENDERN", "IZQUIERDA DERECHA: CAMBIAR VALOR", "GAUCHE DROITE: CHANGER LA VALEUR", "SINISTRA DESTRA: CAMBIA VALORE", "ESQUERDA DIREITA: MUDAR VALOR"}},  // FChange
    {{"OPEN CONTROLS OR EDIT RGB", "ОТКРЫТЬ УПРАВЛЕНИЕ ИЛИ ПРАВИТЬ RGB", "ВІДКРИТИ КЕРУВАННЯ АБО ПРАВИТИ RGB", "STEUERUNG OEFFNEN ODER RGB AENDERN", "ABRIR CONTROLES O EDITAR RGB", "OUVRIR COMMANDES OU EDITER RGB", "APRI COMANDI O MODIFICA RGB", "ABRIR CONTROLES OU EDITAR RGB"}},  // FConfirm
    {{"CLOSE AND SAVE", "ЗАКРЫТЬ И СОХРАНИТЬ", "ЗАКРИТИ І ЗБЕРЕГТИ", "SCHLIESSEN UND SPEICHERN", "CERRAR Y GUARDAR", "FERMER ET ENREGISTRER", "CHIUDI E SALVA", "FECHAR E SALVAR"}},  // FBack
    {{"PIECE SETS: FOLDER PIECES/NAME", "НАБОРЫ ФИГУР: ПАПКА PIECES/ИМЯ", "НАБОРИ ФІГУР: ТЕКА PIECES/НАЗВА", "FIGURENSAETZE: ORDNER PIECES/NAME", "JUEGOS DE PIEZAS: CARPETA PIECES/NOMBRE", "JEUX DE PIECES: DOSSIER PIECES/NOM", "SET DI PEZZI: CARTELLA PIECES/NOME", "CONJUNTOS DE PECAS: PASTA PIECES/NOME"}},  // FPieceFolder
    {{"LIGHT SQUARE", "СВЕТЛАЯ КЛЕТКА", "СВІТЛА КЛІТИНКА", "HELLES FELD", "CASILLA CLARA", "CASE CLAIRE", "CASA CHIARA", "CASA CLARA"}},  // ELight
    {{"DARK SQUARE", "ТЁМНАЯ КЛЕТКА", "ТЕМНА КЛІТИНКА", "DUNKLES FELD", "CASILLA OSCURA", "CASE FONCEE", "CASA SCURA", "CASA ESCURA"}},  // EDark
    {{"ACCENT", "АКЦЕНТ", "АКЦЕНТ", "AKZENT", "ACENTO", "ACCENT", "ACCENTO", "DESTAQUE"}},  // EAccent
    {{"BACKGROUND", "ФОН", "ФОН", "HINTERGRUND", "FONDO", "FOND", "SFONDO", "FUNDO"}},  // EBackground
    {{"EDIT", "ПРАВКА", "ПРАВКА", "ANPASSEN", "EDITAR", "EDITER", "MODIFICA", "EDITAR"}},  // EEdit
    {{"RED", "КРАСНЫЙ", "ЧЕРВОНИЙ", "ROT", "ROJO", "ROUGE", "ROSSO", "VERMELHO"}},  // ERed
    {{"GREEN", "ЗЕЛЁНЫЙ", "ЗЕЛЕНИЙ", "GRUEN", "VERDE", "VERT", "VERDE", "VERDE"}},  // EGreen
    {{"BLUE", "СИНИЙ", "СИНІЙ", "BLAU", "AZUL", "BLEU", "BLU", "AZUL"}},  // EBlue
    {{"UP DOWN: CHANNEL  LEFT RIGHT: VALUE", "ВВЕРХ ВНИЗ: КАНАЛ  ВЛЕВО ВПРАВО: ЗНАЧЕНИЕ", "ВГОРУ ВНИЗ: КАНАЛ  ВЛІВО ВПРАВО: ЗНАЧЕННЯ", "HOCH RUNTER: KANAL  LINKS RECHTS: WERT", "ARRIBA ABAJO: CANAL  IZQ DER: VALOR", "HAUT BAS: CANAL  GAUCHE DROITE: VALEUR", "SU GIU: CANALE  SX DX: VALORE", "CIMA BAIXO: CANAL  ESQ DIR: VALOR"}},  // EHelp
    {{"SAVES", "СОХРАНЯЕТ", "ЗБЕРІГАЄ", "SPEICHERT", "GUARDA", "ENREGISTRE", "SALVA", "SALVA"}},  // ESave
    {{"CLOSE HELP", "ЗАКРЫТЬ ПОМОЩЬ", "ЗАКРИТИ ДОВІДКУ", "HILFE SCHLIESSEN", "CERRAR AYUDA", "FERMER AIDE", "CHIUDI AIUTO", "FECHAR AJUDA"}},  // HelpClose
    {{"MOVE CURSOR", "ДВИГАТЬ КУРСОР", "РУХАТИ КУРСОР", "CURSOR BEWEGEN", "MOVER CURSOR", "DEPLACER CURSEUR", "MUOVI CURSORE", "MOVER CURSOR"}},  // HelpCursor
    {{"CURSOR UP", "КУРСОР ВВЕРХ", "КУРСОР ВГОРУ", "KURSOR HOCH", "CURSOR ARRIBA", "CURSEUR HAUT", "CURSORE SU", "CURSOR CIMA"}},  // AUp
    {{"CURSOR DOWN", "КУРСОР ВНИЗ", "КУРСОР ВНИЗ", "KURSOR RUNTER", "CURSOR ABAJO", "CURSEUR BAS", "CURSORE GIU", "CURSOR BAIXO"}},  // ADown
    {{"CURSOR LEFT", "КУРСОР ВЛЕВО", "КУРСОР ВЛІВО", "KURSOR LINKS", "CURSOR IZQUIERDA", "CURSEUR GAUCHE", "CURSORE SINISTRA", "CURSOR ESQUERDA"}},  // ALeft
    {{"CURSOR RIGHT", "КУРСОР ВПРАВО", "КУРСОР ВПРАВО", "KURSOR RECHTS", "CURSOR DERECHA", "CURSEUR DROITE", "CURSORE DESTRA", "CURSOR DIREITA"}},  // ARight
    {{"SELECT OR MOVE", "ВЫБРАТЬ ИЛИ ХОДИТЬ", "ОБРАТИ АБО ХОДИТИ", "WAEHLEN ODER ZIEHEN", "ELEGIR O MOVER", "CHOISIR OU JOUER", "SCEGLI O MUOVI", "ESCOLHER OU MOVER"}},  // AConfirm
    {{"UNDO OR CANCEL", "ОТМЕНИТЬ ХОД ИЛИ ВЫБОР", "СКАСУВАТИ ХІД АБО ВИБІР", "ZUG ZURUECK ODER ABBRUCH", "DESHACER O CANCELAR", "ANNULER COUP OU CHOIX", "ANNULLA MOSSA O SCELTA", "DESFAZER OU CANCELAR"}},  // ABack
    {{"TOGGLE 2D 3D", "ПЕРЕКЛЮЧИТЬ 2D 3D", "ПЕРЕМКНУТИ 2D 3D", "2D 3D UMSCHALTEN", "CAMBIAR 2D 3D", "BASCULER 2D 3D", "CAMBIA 2D 3D", "ALTERNAR 2D 3D"}},  // AView
    {{"CAMERA LEFT", "КАМЕРА ВЛЕВО", "КАМЕРА ВЛІВО", "KAMERA LINKS", "CAMARA IZQUIERDA", "CAMERA GAUCHE", "CAMERA SINISTRA", "CAMERA ESQUERDA"}},  // ACamLeft
    {{"CAMERA RIGHT", "КАМЕРА ВПРАВО", "КАМЕРА ВПРАВО", "KAMERA RECHTS", "CAMARA DERECHA", "CAMERA DROITE", "CAMERA DESTRA", "CAMERA DIREITA"}},  // ACamRight
    {{"EXIT GAME", "ВЫЙТИ ИЗ ИГРЫ", "ВИЙТИ З ГРИ", "SPIEL BEENDEN", "SALIR DEL JUEGO", "QUITTER", "ESCI DAL GIOCO", "SAIR DO JOGO"}},  // AQuit
    {{"PRESS A BUTTON FOR", "НАЖМИТЕ КНОПКУ ДЛЯ", "НАТИСНІТЬ КНОПКУ ДЛЯ", "TASTE DRUECKEN FUER", "PULSA UN BOTON PARA", "APPUYEZ SUR UN BOUTON POUR", "PREMI UN TASTO PER", "PRESSIONE UM BOTAO PARA"}},  // CPress
    {{"AUTO CANCEL IN", "АВТООТМЕНА ЧЕРЕЗ", "АВТОСКАСУВАННЯ ЧЕРЕЗ", "ABBRUCH IN", "CANCELA EN", "ANNULATION DANS", "ANNULLA TRA", "CANCELA EM"}},  // CWait
    {{"RESET TO DEFAULT", "ВЕРНУТЬ СТАНДАРТНЫЕ", "ПОВЕРНУТИ СТАНДАРТНІ", "STANDARD WIEDERHERSTELLEN", "RESTABLECER", "REINITIALISER", "RIPRISTINA", "RESTAURAR PADRAO"}},  // CReset
    {{"NOT SET", "НЕ ЗАДАНО", "НЕ ЗАДАНО", "NICHT BELEGT", "SIN ASIGNAR", "NON DEFINI", "NON ASSEGNATO", "SEM ATRIBUICAO"}},  // CNone
    {{"KEYBOARD KEYS ALWAYS WORK", "КЛАВИШИ КЛАВИАТУРЫ РАБОТАЮТ ВСЕГДА", "КЛАВІШІ КЛАВІАТУРИ ПРАЦЮЮТЬ ЗАВЖДИ", "TASTATUR FUNKTIONIERT IMMER", "EL TECLADO SIEMPRE FUNCIONA", "LE CLAVIER FONCTIONNE TOUJOURS", "LA TASTIERA FUNZIONA SEMPRE", "O TECLADO SEMPRE FUNCIONA"}},  // CFoot1
    {{"A TAKEN BUTTON SWAPS PLACES", "ЗАНЯТАЯ КНОПКА МЕНЯЕТСЯ МЕСТАМИ", "ЗАЙНЯТА КНОПКА МІНЯЄТЬСЯ МІСЦЯМИ", "BELEGTE TASTE WIRD GETAUSCHT", "UN BOTON USADO SE INTERCAMBIA", "UN BOUTON PRIS EST ECHANGE", "UN TASTO USATO VIENE SCAMBIATO", "UM BOTAO USADO E TROCADO"}},  // CFoot2
}};

int currentLanguage = 0;

}  // namespace

const char* languageCode(int index) {
    return Languages[static_cast<std::size_t>(index >= 0 && index < LanguageCount ? index : 0)].code;
}

const char* languageName(int index) {
    return Languages[static_cast<std::size_t>(index >= 0 && index < LanguageCount ? index : 0)].name;
}

int languageFromCode(const std::string& code) {
    for (int i = 0; i < LanguageCount; ++i) {
        if (code == Languages[static_cast<std::size_t>(i)].code) return i;
    }
    return -1;
}

void setLanguage(int index) { currentLanguage = index >= 0 && index < LanguageCount ? index : 0; }

int language() { return currentLanguage; }

const char* trIn(Str key, int languageIndex) {
    const auto& row = Table[static_cast<std::size_t>(key)];
    const char* text = row[static_cast<std::size_t>(languageIndex >= 0 && languageIndex < LanguageCount ? languageIndex : 0)];
    return text != nullptr && text[0] != '\0' ? text : row[0];
}

const char* tr(Str key) { return trIn(key, currentLanguage); }

}  // namespace chessforge

/**
 * ============================================================================
 * Projeto: Kodanima (Editor de Animação 2D Frame a Frame)
 * Descrição: Desenvolvido em C++ puro utilizando Win32 API e GDI.
 * Arquitetura: Janelas Modulares, Cache de Histórico configurável (16MB-200MB, 5-50 ações) para Ctrl+Z/Y,
 *             Persistência BMP e compatibilidade com projetos legados.
 * Compatibilidade: Windows XP até Windows 11 (Compilável via MinGW-W64 e Geany).
 * ============================================================================
 */

#define _WIN32_WINNT 0x0501
#define NOMINMAX
#include <windows.h>
#ifndef BI_ALPHABITFIELDS
// Valor definido pelo Windows SDK mais recente; alguns SDKs antigos do MinGW não o declaram.
#define BI_ALPHABITFIELDS 6
#endif
#include <windowsx.h>
#include <d3d9.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dsound.h>
#include <algorithm>
#include <commdlg.h>
#include <commctrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#include <shlobj.h>
#include <shellapi.h>
#include <setupapi.h> // structs/flags; API calls are resolved dynamically to avoid an extra link dependency
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <list>
#include <map>
#include <unordered_map>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <locale>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cstddef>
#include <new>
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <cpuid.h>
#include <immintrin.h>
#define KODA_GNU_X86_SIMD 1
#else
#define KODA_GNU_X86_SIMD 0
#endif

// Compatibilidade com SDKs/headers antigos usados ao compilar para Windows XP.
#ifndef OCR_NORMAL
#define OCR_NORMAL 32512
#endif
#ifndef CSIDL_MYVIDEO
#define CSIDL_MYVIDEO 0x000e
#endif

// Declaracoes antecipadas para callbacks de EnumChildWindows (WNDENUMPROC).
BOOL CALLBACK AplicarFonteControle(HWND child, LPARAM p);
static void AplicarCategoriaPreferencias(HWND hwnd, int categoria);
static void AtualizarPainelCPU(HWND hwnd);
static int DetectarMelhorMotorGrafico();
static bool VerificarDirectX11Disponivel();
static bool VerificarDirectX12Disponivel();
static void AtualizarListaAtalhos(HWND hwnd, int indiceSelecionado);
void AbrirPainelAtalhos();
LRESULT CALLBACK WndProcShortcuts(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void AbrirGerenciadorPincel(HWND owner);
static void KodaCriarDicasFerramentas(HWND parent);
static HBITMAP BitmapCamadaAtual();
static bool ScreenToBitmapPoint(HWND hwnd,HBITMAP hb,POINT p,int& x,int& y);
static HBITMAP CriarPreviewVermelho(HBITMAP source);
static void DesenharLinhaAntiAliased(HDC dc, int x1, int y1, int x2, int y2, COLORREF cor, float largura, int dureza);
static void DesenharLinhaComPontaBitmap(HDC dc,int x1,int y1,int x2,int y2,COLORREF cor,float largura);
static void AtivarBrushSelecionado(int index);
LRESULT CALLBACK WndProcBrushManager(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// IDs de Controles e Janelas
#define ID_BTN_CRIAR          101
#define ID_BTN_ABRIR          102
#define ID_BTN_PROSSEGUIR     103
#define ID_BTN_SELECIONAR_DIR 104

#define ID_EDIT_LARGURA       201
#define ID_EDIT_ALTURA        202
#define ID_EDIT_FPS           203
#define ID_EDIT_NOME          204
#define ID_EDIT_DIR           205

#define ID_TOOL_LAPIS         301
#define ID_TOOL_BORRACHA      302
#define ID_TOOL_BALDE         303
#define ID_TOOL_LACON         304
#define ID_TOOLSETTINGS_CLOSE  439
#define ID_TOOL_RODA_CORES    305
#define ID_TOOL_CONTA_GOTAS   306
#define ID_SETUP_PRESET      206
#define ID_SETUP_DPI         207
#define ID_SETUP_FULLPATH    208
#define ID_TIMELINE_FOLLOW_THEME 461
#define ID_TIMELINE_CUSTOM_COLOR 462
#define ID_STARTUP_GPT_LABEL     463
#define ID_BTN_LANGUAGE          464
#define ID_TIMELINE_IMPORT       7420
#define ID_TIMELINE_AUDIO_IMPORT ID_TIMELINE_IMPORT /* compatibilidade interna */
#define ID_TIMELINE_AUDIO_SCRUB  7421
#define ID_TIMELINE_IMPORT_IMAGE 7423 /* ID antigo, não cria mais um botão */
#define ID_RENDER_AUDIO          7422
#define ID_LANGUAGE_LIST         465
#define ID_LANGUAGE_APPLY        466
#define ID_LANGUAGE_CANCEL       467
#define ID_LANGUAGE_DESCRIPTION  468

#define ID_BTN_PLAY           401
#define ID_BTN_ONION          402
#define ID_BTN_OPAC_ONION     403
#define ID_BTN_NOVO_QUADRO    404
#define ID_BTN_RENDERIZAR     405
#define ID_BTN_FRAME_BASE     500
#define ID_BTN_FRAME_PREV      407
#define ID_BTN_FRAME_NEXT      408
#define ID_EDIT_FRAME_SECONDS  409
#define ID_UPDOWN_FRAME_SEC    410
#define ID_BTN_INSERIR_MEIO    411
#define ID_BTN_PLAY_PREVIEW    412
#define ID_BTN_SETTINGS        420
#define ID_SETTINGS_THEME      421
#define ID_SETTINGS_CURSOR     422
#define ID_SETTINGS_SIZE       423
#define ID_SETTINGS_GAP        424
#define ID_SETTINGS_PRESSURE   425
#define ID_SETTINGS_SAVE       426
#define ID_SETTINGS_RESET      427
#define ID_TOOLSETTINGS_SIZE   428
#define ID_TOOLSETTINGS_PRESS  429
#define ID_TOOLSETTINGS_GAP    430
#define ID_TOOLSETTINGS_TOL    431
#define ID_TOOLSETTINGS_MINPRESS 432
#define ID_TOOLSETTINGS_MAXPRESS 433
#define ID_TOOLSETTINGS_ERASER_SIZE 451
#define ID_TOOLSETTINGS_ERASER_PRESS 452
#define ID_TOOLSETTINGS_ERASER_MIN 453
#define ID_TOOLSETTINGS_ERASER_MAX 454
#define ID_TOOLSETTINGS_LASSO_MODE 455
#define ID_TOOLSETTINGS_LASSO_COLOR 456
#define ID_TOOLSETTINGS_OK 457
#define ID_TOOLSETTINGS_COPY 458
#define ID_TOOLSETTINGS_CUT 459
#define ID_TOOLSETTINGS_TRANSFORM 460
#define ID_TOOLSETTINGS_ANTIALIAS 601
#define ID_TOOLSETTINGS_HARDNESS1 602
#define ID_TOOLSETTINGS_HARDNESS2 603
#define ID_TOOLSETTINGS_HARDNESS3 604
#define ID_TOOLSETTINGS_HARDNESS4 605
#define ID_TOOLSETTINGS_SPACING 606
#define ID_TOOLSETTINGS_ANGLE 607
#define ID_TOOLSETTINGS_ROUNDNESS 608
#define ID_TOOLSETTINGS_SCATTER 609
#define ID_TOOLSETTINGS_OPACITY 610
#define ID_TOOLSETTINGS_FLOW 611
#define ID_TOOLSETTINGS_ROTATE_STROKE 612
#define ID_TOOLSETTINGS_SIZE_DYNAMICS 613
#define ID_TOOLSETTINGS_OPACITY_DYNAMICS 614
#define ID_BRUSH_LIST 620
#define ID_BRUSH_IMPORT 621
#define ID_BRUSH_DELETE 622
#define ID_BRUSH_SETTINGS 623
#define ID_BRUSH_TEXTURE  625
#define ID_BRUSH_CLOSE 624
#define ID_BRUSH_NAME_EDIT 626
#define ID_BRUSH_RENAME 627
#define ID_BRUSH_MKV 628
#define ID_BRUSH_PROP_NAME 640
#define ID_BRUSH_PROP_SIZE 641
#define ID_BRUSH_PROP_MIN 642
#define ID_BRUSH_PROP_MAX 643
#define ID_BRUSH_PROP_SPACING 644
#define ID_BRUSH_PROP_ANGLE 645
#define ID_BRUSH_PROP_SCATTER 646
#define ID_BRUSH_PROP_OPACITY 647
#define ID_BRUSH_PROP_FLOW 648
#define ID_BRUSH_PROP_STARTSIZE 649
#define ID_BRUSH_PROP_ENDSIZE 650
#define ID_BRUSH_PROP_STARTOPACITY 651
#define ID_BRUSH_PROP_ENDOPACITY 652
#define ID_BRUSH_PROP_SPRAYDENSITY 653
#define ID_BRUSH_PROP_SPRAYRADIUS 654
#define ID_BRUSH_PROP_PARTICLES 655
#define ID_BRUSH_PROP_SPREAD 656
#define ID_BRUSH_PROP_ROTATE 657
#define ID_BRUSH_PROP_RANDOMROT 658
#define ID_BRUSH_PROP_SPRAY 659
#define ID_BRUSH_PROP_PARTICLESON 660
#define ID_BRUSH_PROP_FOLLOW 661
#define ID_BRUSH_PROP_ANTIOVERFLOW 662
#define ID_BRUSH_PROP_OPTIMIZE 663
#define ID_BRUSH_PROP_SIZEPRESS 664
#define ID_BRUSH_PROP_OPACITYPRESS 665
#define ID_BRUSH_PROP_STARTEND 666
#define ID_BRUSH_PROP_TEXTURE2 667
#define ID_BRUSH_PROP_TEXTURE3 668
#define ID_BRUSH_PROP_SAVE 669
#define ID_BRUSH_PROP_CANCEL 670
#define ID_BRUSH_PROP_TINT2 671
#define ID_BRUSH_PROP_TINT3 672
#define ID_BRUSH_PROP_HARDNESS 673
#define ID_BRUSH_PROP_ROUNDNESS 674
#define ID_BRUSH_PROP_ANTIALIAS 675
#define ID_BRUSH_PROP_BLUR 676
#define ID_BRUSH_PROP_SCALERAND 677
#define ID_TOOLSETTINGS_STABILIZER_ENABLE 689
#define ID_TOOLSETTINGS_STABILIZER_LEVEL  690
#define ID_BRUSH_PAGE_TIP 680
#define ID_BRUSH_PAGE_STROKE 681
#define ID_BRUSH_PAGE_TEXTURE 682
#define ID_BRUSH_PAGE_SPRAY 683
#define ID_BRUSH_PAGE_STARTEND 684
#define ID_BRUSH_PAGE_PRESSURE 685
#define ID_BRUSH_PAGE_SAFETY 686
#define ID_BRUSH_PAGE_MKV 687
#define ID_BRUSH_PAGE_BACK 688
#define ID_BRUSH_USE_IMPORTED 629
#define ID_BTN_DELETE_FRAME    413
#define ID_LAYER_ADD           414
#define ID_LAYER_DELETE        415
#define ID_LAYER_UP            416
#define ID_LAYER_DOWN          417
#define ID_SETTINGS_RENDERER   432
#define ID_SETTINGS_UI_SCALE   433
#define ID_SETTINGS_TEXT_SIZE  434
#define ID_SETTINGS_CORES       435
#define ID_SETTINGS_RAM         436
#define ID_EDIT_ONION_OPACITY   437
#define ID_EDIT_LAYER_RENAME    438
#define ID_DURATION_UNIT        439
#define ID_SETTINGS_TIMELINE_BUTTONS 440
#define ID_EDIT_FPS_TIMELINE    441
#define ID_SETTINGS_LAYER_HEIGHT 442
#define ID_SETTINGS_RULER_HEIGHT 443
#define ID_SETTINGS_PRESSURE_API 444
#define ID_SETTINGS_HISTORY_ACTIONS 445
#define ID_SETTINGS_HISTORY_CACHE  446
#define ID_BTN_LOOP_RANGE      447
#define ID_SETTINGS_HOTKEY_LIST 470
#define ID_SETTINGS_HOTKEY_KEY 471
#define ID_SETTINGS_HOTKEY_APPLY 472
#define ID_SETTINGS_HOTKEY_SAVE 473
#define ID_SETTINGS_HOTKEY_RESET 474
#define ID_SETTINGS_CATEGORY_TITLE 478
#define ID_SETTINGS_CREATOR_LABEL 479
#define ID_SETTINGS_CPU_INFO       7600
#define ID_SETTINGS_CPU_COUNT      7601
#define ID_SETTINGS_CPU_MODE       7602
#define ID_SETTINGS_CPU_MMX        7603
#define ID_SETTINGS_CPU_SSE        7604
#define ID_SETTINGS_CPU_SSE2       7605
#define ID_SETTINGS_CPU_SSE3       7606
#define ID_SETTINGS_CPU_SSSE3      7607
#define ID_SETTINGS_CPU_SSE41      7608
#define ID_SETTINGS_CPU_SSE42      7609
#define ID_SETTINGS_CPU_AVX        7610
#define ID_SETTINGS_CPU_AVX2       7611
#define ID_SETTINGS_CPU_AES        7612
#define ID_SETTINGS_CPU_VIRTUAL    7613
#define ID_SETTINGS_LABEL_THEME 510
#define ID_SETTINGS_LABEL_RENDERER 511
#define ID_SETTINGS_LABEL_UI_SCALE 512
#define ID_SETTINGS_LABEL_TEXT_SIZE 513
#define ID_SETTINGS_LABEL_TIMELINE_BUTTONS 514
#define ID_SETTINGS_LABEL_LAYER_HEIGHT 515
#define ID_SETTINGS_LABEL_RULER_HEIGHT 516
#define ID_SETTINGS_LABEL_CORES 517
#define ID_SETTINGS_LABEL_RAM 518
#define ID_SETTINGS_LABEL_PRESSURE_API 519
#define ID_SETTINGS_LABEL_HISTORY_ACTIONS 520
#define ID_SETTINGS_LABEL_HISTORY_CACHE 521
#define ID_SETTINGS_LABEL_SYSTEM 522
#define ID_SETTINGS_LABEL_HISTORY_NOTE 523
#define ID_SETTINGS_LABEL_CONFIG_PATH 524
#define ID_SETTINGS_HEADER 525
#define ID_SETTINGS_LABEL_TABLET_NOTE 526
#define ID_SETTINGS_TABLET_DEVICE_VALUE 7520
#define ID_SETTINGS_TABLET_DRIVER_VALUE 7521
#define ID_SETTINGS_TABLET_LEVELS_VALUE 7522
#define ID_SETTINGS_TABLET_NAME_EDIT 7523
#define ID_SETTINGS_TABLET_LEVELS_EDIT 7524
#define ID_SETTINGS_TABLET_APPLY 7525
#define ID_SETTINGS_TABLET_LABEL_DEVICE 7526
#define ID_SETTINGS_TABLET_LABEL_DRIVER 7527
#define ID_SETTINGS_TABLET_LABEL_LEVELS 7528
#define ID_SETTINGS_TABLET_LABEL_MANUAL_NAME 7529
#define ID_SETTINGS_TABLET_LABEL_MANUAL_LEVELS 7530
#define ID_SETTINGS_TABLET_NOTE 7531
#define ID_SETTINGS_LABEL_SHORTCUT_NOTE 527
#define ID_SETTINGS_EXPLANATION_TEXT 7532
#define ID_TIMER_PREVIEW       7001

struct ProjectConfig {
    int largura = 800;
    int altura = 600;
    int fps = 24;
    COLORREF corFundo = RGB(255, 255, 255);
    wchar_t diretorioPasta[MAX_PATH] = L"";
    wchar_t nomeProjeto[256] = L"MeuProjetoAnimacao";
};

// Estrutura do Cache de Histórico (Undo / Redo entre 1MB e 200MB)
struct HistoryState {
    std::vector<BYTE> pixelData;
    int largura;
    int altura;
};

// Histórico do canvas/timeline: limite de ações e teto de memória configuráveis.
// Padrão: 20 ações e 200 MB, persistidos em Documentos\Kodanima\config.ini.
int historyActionLimit = 20;
int historyCacheLimitMB = 200;

struct FrameData {
    int id;
    double segundos = 1.0;
    int unidadeDuracao = 1; // 0=minutos, 1=segundos
    HBITMAP hBmpQuadro = NULL;
    std::list<HistoryState> undoStack;
    std::list<HistoryState> redoStack;
    size_t cacheMemoryUsageBytes = 0;
    std::vector<HBITMAP> layerBitmaps; // bitmaps independentes por camada; hBmpQuadro aponta para a camada ativa
    std::vector<double> layerDurations; // duração independente por camada neste quadro
    std::vector<bool> layerFrameExists; // quadro existe apenas nas camadas marcadas como true
};

// Variáveis Globais
ProjectConfig configAtual;
HINSTANCE hInstGlobal = NULL;
HWND hwndMain = NULL;
static const UINT_PTR ID_TIMER_AUTO_HOVER_FOCUS = 0x4B31;
static const UINT_PTR ID_TIMER_STARTUP_ANIMATION = 0x4B32;
static int g_startupAnimType = 0; // 1 = novo projeto (+), 2 = abrir projeto (olho), 3 = Pac-Man/idioma
static std::wstring g_languageCode = L"pt-BR";
static HWND g_hwndLanguageDialog = NULL;
// Faixa de áudio da timeline. A decodificação/reprodução é delegada aos codecs
// instalados no Windows via MCI; não mantém áudio PCM inteiro na memória.
static std::wstring g_audioPath;
static std::wstring g_audioRelativePath; // caminho relativo ao projeto: assets/media_XXXX.ext
static double g_audioStartSeconds = 0.0;
static double g_audioDurationSeconds = 0.0;
struct KodaProjectMediaAsset {
    std::wstring name; // nome original exibido na timeline e salvo no project.json
    std::wstring relativePath;
    std::wstring type; // image, audio, video ou gif
    double startSeconds = 0.0;
    double durationSeconds = 0.0;
    int frame = 0;
    int layer = 0;
    int x = 0;
    int y = 0;
};
static std::vector<KodaProjectMediaAsset> g_projectMediaAssets;
struct KodaDeletedMediaAsset { KodaProjectMediaAsset asset; DWORD deletedAt=0; };
static std::vector<KodaDeletedMediaAsset> g_deletedMediaAssets; // janela de restauração de 30 s
static int g_selectedMediaAssetIndex = -1;
static const UINT_PTR ID_TIMER_MEDIA_ASSET_PURGE = 0x4B34;
static std::wstring g_pendingImageAssetRelativePath;
static bool g_visualClipDragging = false;
static int g_visualClipDragIndex = -1; // índice dentro de g_projectMediaAssets
static double g_visualClipDragGrabOffset = 0.0;
static bool g_audioDeviceOpen = false;
static bool g_audioPlayable = false;
static bool g_audioScrubEnabled = true;
static bool g_audioClipDragging = false;
static double g_audioDragGrabOffset = 0.0;
static DWORD g_audioLastScrubTick = 0;
static const UINT_PTR ID_TIMER_AUDIO_SCRUB_STOP = 0x4B33;
static bool g_audioBackendDirectSound = false;
static HMODULE g_directSoundModule = NULL;
static LPDIRECTSOUND8 g_directSoundDevice = NULL;
static LPDIRECTSOUNDBUFFER g_directSoundBuffer = NULL;
static WAVEFORMATEX g_directSoundWaveFormat{};
static DWORD g_directSoundDataBytes = 0;
typedef HRESULT (WINAPI *KodaDirectSoundCreate8Proc)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);
static KodaDirectSoundCreate8Proc g_directSoundCreate8 = NULL;
typedef UINT (WINAPI *KodaMciSendStringWProc)(LPCWSTR, LPWSTR, UINT, HWND);
static HMODULE g_mciModule = NULL;
static KodaMciSendStringWProc g_mciSendStringW = NULL;
static bool KodaAudioAbrir(HWND owner, bool avisar);
static void KodaAudioParar();
static void KodaAudioFechar();
static void KodaAudioTocarDaTimeline(double segundos);
static void KodaAudioScrub(double segundos);
static int g_startupAnimFrame = 0;
static bool g_startupAnimationDispatch = false;
static RECT g_startupCreateBaseRect = {0,0,0,0};
static RECT g_startupOpenBaseRect = {0,0,0,0};
static wchar_t g_pastaInicialAbrir[MAX_PATH] = L"";
HWND g_hoverFocusedRoot = NULL;
HWND hwndCanvas = NULL;
HWND hwndTimeline = NULL;
HWND hwndTools = NULL;
// A borda da barra de tarefas em ocultação automática é verificada uma vez
// quando o canvas é aberto. A maximização reserva então 2 px nessa borda.
static int g_canvasAutoHideTaskbarEdge = -1;
static bool g_canvasTaskbarInsetChecked = false;
HWND hwndEditDir = NULL;
HICON hIconeApp = NULL;

std::vector<FrameData> listaFrames;
struct TimelineLayer {
    std::wstring name;
    bool visible;
    bool onionEnabled;
    TimelineLayer() : name(L"Camada 1"), visible(true), onionEnabled(true) {}
    TimelineLayer(const std::wstring& layerName, bool isVisible = true)
        : name(layerName), visible(isVisible), onionEnabled(true) {}
};
std::vector<TimelineLayer> timelineLayers = {{L"Camada 1", true}};
int activeTimelineLayer = 0;
int selectedTimelineLayer = 0;
int selectedTimelineFrame = 0;
int timelineScrollSeconds = 0;
int tamanhoBotoesTimeline = 80;
int alturaLinhaCamada = 27;
int alturaReguaTimeline = 24;
const int TIMELINE_RULER_TOP = 72; // faixa vertical compacta abaixo dos controles
const int TIMELINE_SPLIT_X = 184; // divisor compacto entre camadas e régua (2 px de respiro)
int timelineLayerScroll = 0;
int timelineDragStartY = 0;
bool timelineDraggingResize = false;
int timelineDefaultHeight = 285;
int escalaBotoesTimelineAplicada = 100;
struct TimelineControlBaseRect { HWND hwnd; RECT rect; };
std::vector<TimelineControlBaseRect> g_timelineControlBaseRects;
POINT g_ultimaPosicaoMouseFoco = {-32768, -32768};
int g_ticksMouseParado = 0;
bool g_timerFocoAtivo = false;
int durationUnit = 1; // 0=minutos, 1=segundos
HWND hwndLayerRenameEdit = NULL;
int layerRenameIndex = -1;
struct TLFrameSnapshot { double seconds; int durationUnit = 1; std::vector<double> layerDurations; std::vector<bool> layerFrameExists; std::vector<HBITMAP> bitmaps; };
struct TimelineSnapshot { std::vector<TimelineLayer> layers; std::vector<TLFrameSnapshot> frames; int activeLayer=0, selectedLayer=0, currentFrame=0; };
std::vector<TimelineSnapshot> timelineUndoStack, timelineRedoStack;
bool timelineHistoryRestoring = false;
double previewTimeSeconds = 0.0;
bool timelinePlayheadDragging = false;
double timelinePlayheadSeconds = 0.0;
bool timelineLoopEnabled = false;
double timelineLoopStartSeconds = 0.0;
double timelineLoopEndSeconds = 0.0;
int timelineLoopMarkerDragging = 0; // 1=início, 2=fim
// Gestos da régua: botão direito ou caneta pressionada.
int timelineDurationDragFrame = -1;
static bool g_timelineDurationTeleportPending = false;
static POINT g_timelineDurationTeleportPoint = {-1, -1};
int timelineReorderSourceFrame = -1;
int timelineReorderLayer = -1;
bool timelineFrameReorderDragging = false;
bool g_timelinePenPlayheadGesture = false;

int temaAtual = 0; // 0 escuro, 1 claro, 2 pink, 3 preto/roxo
float tamanhoLapis = 2.0f;
int nivelGapBalde = 8;
int toleranciaBalde = 3; // fecha pequenas falhas no contorno, em pixels
bool pressaoCanetaAtiva = false;
// 0 = Wintab32.dll, 1 = Windows Pointer / Ink (Windows 8+)
int pressureApi = 0;
struct KodaShortcut {
    const wchar_t* name;
    UINT vk;
    UINT defaultVk;
    bool fixed;
    bool ctrl;
    bool shift;
    // Construtor explícito mantém compatibilidade com MinGW/GCC em C++17.
    KodaShortcut(const wchar_t* n, UINT key, UINT def, bool isFixed,
                 bool useCtrl = false, bool useShift = false)
        : name(n), vk(key), defaultVk(def), fixed(isFixed),
          ctrl(useCtrl), shift(useShift) {}
};
static KodaShortcut g_kodaShortcuts[] = {
    {L"Quadro anterior", VK_OEM_COMMA, VK_OEM_COMMA, false},
    {L"Próximo quadro", VK_OEM_PERIOD, VK_OEM_PERIOD, false},
    {L"Play / Pausar", VK_RETURN, VK_RETURN, false},
    {L"Abrir preferências", VK_F1, VK_F1, false},
    {L"Inserir quadro no meio", VK_F7, VK_F7, false},
    {L"Maximizar / restaurar canvas", VK_TAB, VK_TAB, false},
    {L"Excluir quadro/elemento", VK_DELETE, VK_DELETE, false},
    {L"Cancelar seleção / laço", VK_F11, VK_F11, true},
    {L"Criar novo quadro", VK_F2, VK_F2, false},
    {L"Alternar Onion Skin", VK_F3, VK_F3, false},
    {L"Exportar / renderizar", VK_F4, VK_F4, false},
    {L"Ferramenta lápis", VK_F5, VK_F5, false},
    {L"Ferramenta borracha", VK_F8, VK_F8, false},
    {L"Ferramenta balde", VK_F9, VK_F9, false},
    {L"Roda de cores", VK_F10, VK_F10, false},
    {L"Conta-gotas", VK_F12, VK_F12, false},
    {L"Adicionar camada", VK_HOME, VK_HOME, false},
    {L"Remover camada", VK_END, VK_END, false},
    {L"Mover camada para cima", VK_PRIOR, VK_PRIOR, false},
    {L"Mover camada para baixo", VK_NEXT, VK_NEXT, false},
    {L"Importar arquivo (imagem/áudio/vídeo)", 'I', 'I', false},
    {L"Colar imagem da área de transferência", 'V', 'V', true, true, true},
    {L"Aumentar duração do quadro (+ 0,1 s)", VK_ADD, VK_ADD, false},
    {L"Diminuir duração do quadro (- 0,1 s)", VK_SUBTRACT, VK_SUBTRACT, false},
    {L"Mover quadro selecionado para esquerda", VK_NUMPAD4, VK_NUMPAD4, false},
    {L"Mover quadro selecionado para direita", VK_NUMPAD6, VK_NUMPAD6, false}
};
static const int g_kodaShortcutCount = sizeof(g_kodaShortcuts)/sizeof(g_kodaShortcuts[0]);
static int g_selectedShortcut = 0;
static int g_settingsAnimButtonId = -1;
static COLORREF g_settingsAnimColor = RGB(120,70,190);
static const wchar_t* NomeTeclaKoda(UINT vk) {
    static wchar_t b[24];
    if(vk==VK_OEM_COMMA) return L","; if(vk==VK_OEM_PERIOD) return L".";
    if(vk==VK_RETURN) return L"Enter"; if(vk==VK_TAB) return L"Tab"; if(vk==VK_DELETE) return L"Delete"; if(vk==VK_HOME)return L"Home"; if(vk==VK_END)return L"End"; if(vk==VK_PRIOR)return L"PageUp"; if(vk==VK_NEXT)return L"PageDown";
    if(vk==VK_ADD)return L"Num +"; if(vk==VK_SUBTRACT)return L"Num -"; if(vk==VK_NUMPAD4)return L"Num 4"; if(vk==VK_NUMPAD6)return L"Num 6";
    if(vk>=VK_F1 && vk<=VK_F12){wsprintfW(b,L"F%u",(unsigned)(vk-VK_F1+1));return b;}
    if((vk>='A'&&vk<='Z')||(vk>='0'&&vk<='9')){b[0]=(wchar_t)vk;b[1]=0;return b;}
    wsprintfW(b,L"Tecla %u",(unsigned)vk); return b;
}
static UINT VkPorIndiceTeclaKoda(int i) {
    if(i==0)return VK_OEM_COMMA; if(i==1)return VK_OEM_PERIOD; if(i==2)return VK_RETURN; if(i==3)return VK_TAB; if(i==4)return VK_DELETE;
    if(i>=5&&i<=16)return VK_F1+(i-5); if(i>=17&&i<=42)return 'A'+(i-17); if(i>=43&&i<=52)return '0'+(i-43); if(i==53)return VK_HOME; if(i==54)return VK_END; if(i==55)return VK_PRIOR; if(i==56)return VK_NEXT;
    if(i==57)return VK_ADD; if(i==58)return VK_SUBTRACT; if(i==59)return VK_NUMPAD4; if(i==60)return VK_NUMPAD6; return VK_OEM_COMMA;
}

HWND hwndComboPressureAPI = NULL;
float tamanhoMinimoPressao = 0.01f;
float tamanhoMaximoPressao = 12.0f;
float pressaoAtualCaneta = 1.0f;
float pressaoAnteriorCaneta = 1.0f;
bool eventoCanetaSintetico = false;
bool canetaPointerEmContato = false;

// Tipos locais para receber WM_POINTER no Windows 8+ sem exigir SDK novo.
// Mantém a compilação do programa com _WIN32_WINNT=0x0501 (Windows XP).
struct KODANIMA_POINTER_INFO {
    UINT32 pointerType; UINT32 pointerId; UINT32 frameId; UINT32 pointerFlags;
    HANDLE sourceDevice; HWND hwndTarget; POINT ptPixelLocation; POINT ptHimetricLocation;
    POINT ptPixelLocationRaw; POINT ptHimetricLocationRaw; DWORD dwTime; UINT32 historyCount;
    INT32 inputData; DWORD dwKeyStates; UINT64 PerformanceCount; UINT32 ButtonChangeType;
};
struct KODANIMA_POINTER_PEN_INFO {
    KODANIMA_POINTER_INFO pointerInfo; UINT32 penFlags; UINT32 penMask;
    UINT32 pressure; UINT32 rotation; INT32 tiltX; INT32 tiltY;
};
typedef BOOL (WINAPI *KODANIMA_GetPointerPenInfo)(UINT32, KODANIMA_POINTER_PEN_INFO*);
typedef BOOL (WINAPI *KODANIMA_GetPointerPenInfoHistory)(UINT32, UINT32*, KODANIMA_POINTER_PEN_INFO*);
KODANIMA_GetPointerPenInfo g_getPointerPenInfo = NULL;
KODANIMA_GetPointerPenInfoHistory g_getPointerPenInfoHistory = NULL;

// Subconjunto da API Wintab (Wintab32.dll), carregado dinamicamente para não
// obrigar a presença da DLL nem adicionar dependência de link ao executável.
#define KOD_WT_DEFBASE 0x7FF0
#define KOD_WTI_DEFCONTEXT 3
#define KOD_WTI_DEFSYSCTX 4
#define KOD_WTI_DEVICES 100
#define KOD_DVC_NPRESSURE 15
#define KOD_CXO_MESSAGES 0x0004
#define KOD_CXO_CSRMESSAGES 0x0008
#define KOD_WT_PACKET_MSG (KOD_WT_DEFBASE + 0)
#define KOD_WT_PROXIMITY_MSG (KOD_WT_DEFBASE + 5)
#define KOD_WT_CSRCHANGE_MSG (KOD_WT_DEFBASE + 7)
#define KOD_PK_TIME 0x0004
#define KOD_PK_CURSOR 0x0020
#define KOD_PK_BUTTONS 0x0040
#define KOD_PK_X 0x0080
#define KOD_PK_Y 0x0100
#define KOD_PK_NORMAL_PRESSURE 0x0400
struct KOD_WT_AXIS { LONG axMin; LONG axMax; UINT axUnits; LONG axResolution; };
struct KOD_LOGCONTEXTA {
    char lcName[40]; UINT lcOptions,lcStatus,lcLocks,lcMsgBase,lcDevice,lcPktRate;
    DWORD lcPktData,lcPktMode,lcMoveMask,lcBtnDnMask,lcBtnUpMask;
    LONG lcInOrgX,lcInOrgY,lcInOrgZ,lcInExtX,lcInExtY,lcInExtZ;
    LONG lcOutOrgX,lcOutOrgY,lcOutOrgZ,lcOutExtX,lcOutExtY,lcOutExtZ;
    LONG lcSensX,lcSensY,lcSensZ; BOOL lcSysMode;
    INT lcSysOrgX,lcSysOrgY,lcSysExtX,lcSysExtY; LONG lcSysSensX,lcSysSensY;
};
// PACKET layout follows the ascending PK_* bit order, as pktdef.h does in Krita.
struct KOD_WT_PACKET { DWORD pkTime; UINT pkCursor; UINT pkButtons; LONG pkX,pkY; UINT pkNormalPressure; };
typedef UINT (WINAPI *KOD_WTInfoA)(UINT, UINT, LPVOID);
typedef HANDLE (WINAPI *KOD_WTOpenA)(HWND, KOD_LOGCONTEXTA*, BOOL);
typedef BOOL (WINAPI *KOD_WTClose)(HANDLE);
typedef BOOL (WINAPI *KOD_WTPacket)(HANDLE, UINT, LPVOID);
typedef INT (WINAPI *KOD_WTPacketsGet)(HANDLE, int, LPVOID);
typedef BOOL (WINAPI *KOD_WTGetA)(HANDLE, KOD_LOGCONTEXTA*);
typedef BOOL (WINAPI *KOD_WTQueueSizeSet)(HANDLE, int);
typedef int (WINAPI *KOD_WTQueueSizeGet)(HANDLE);
HMODULE g_wintabModule=NULL; HANDLE g_wintabContext=NULL;
KOD_WTInfoA g_wtInfoA=NULL; KOD_WTOpenA g_wtOpenA=NULL; KOD_WTClose g_wtClose=NULL; KOD_WTPacket g_wtPacket=NULL; KOD_WTPacketsGet g_wtPacketsGet=NULL;
KOD_WTGetA g_wtGetA=NULL; KOD_WTQueueSizeSet g_wtQueueSizeSet=NULL; KOD_WTQueueSizeGet g_wtQueueSizeGet=NULL;
KOD_LOGCONTEXTA g_wintabContextConfig{};
LONG g_wintabPressureMin=0, g_wintabPressureMax=1023;
bool g_wintabPressureAxisAvailable=false;

// Identificação da mesa/tablet: a enumeração PnP roda uma vez na inicialização.
// O eixo Wintab real, quando disponível, tem prioridade sobre o perfil estimado.
static std::wstring g_tabletDetectedName=L"Mesa digitalizadora não identificada";
static std::wstring g_tabletManufacturer=L"Não informado";
static std::wstring g_tabletDriverName=L"Não identificado";
static std::wstring g_tabletDriverVersion=L"Não informada";
static std::wstring g_tabletPressureLevelsSource=L"Aguardando detecção";
static int g_tabletPressureLevelsDetected=0;
static int g_tabletManualPressureLevels=0; // 0 = automático; permite perfil/manual entre 2 e 65536 níveis
static std::wstring g_tabletManualDeviceName;
static bool g_tabletLevelsAreEstimated=false;
static bool g_tabletSetupApiScanDone=false;
static HWND g_tabletDeviceValueHwnd=NULL;
static HWND g_tabletDriverValueHwnd=NULL;
static HWND g_tabletLevelsValueHwnd=NULL;
static int g_tabletWintabDeviceIndex=-1;
static void AtualizarPainelTablet();
DWORD g_wintabLastButtons=0;
bool g_wintabButtonsInitialized=false;
POINT g_wintabLastClientPoint{};
bool g_wintabHasLastClientPoint=false;
bool g_wintabSpacePanHoverActive=false;
POINT g_wintabSpacePanLastPoint{};
// Estado independente do canvas: Wintab deve poder clicar em qualquer janela
// com a ponta e manter o alvo pressionado ate a ponta ser levantada.
bool g_wintabContactActive=false;
bool g_wintabCanvasStrokeActive=false;
bool g_wintabExternalMouseActive=false;
bool g_wintabMouseInjectedDown=false;
HWND g_wintabFallbackMouseTarget=NULL;
POINT g_wintabLastScreenPoint{};
bool g_wintabHasLastScreenPoint=false;
HHOOK g_wintabCursorHook=NULL;
extern bool desenhando;
// Declaracoes antecipadas para o backend Wintab, definido antes dos controles da UI.
extern wchar_t caminhoCursor[MAX_PATH];
extern HCURSOR cursorPersonalizado;
static void RestaurarCursorSistemaWintab();
static void AplicarCursorGlobalWintab();

static bool WintabLerEixoPressao(UINT device, KOD_WT_AXIS* axis) {
    if (!g_wtInfoA || !axis || device > 1024u) return false;
    ZeroMemory(axis, sizeof(*axis));
    if (!g_wtInfoA(KOD_WTI_DEVICES + device, KOD_DVC_NPRESSURE, axis)) return false;
    return axis->axMax > axis->axMin;
}

// Respeita a resolução informada pelo eixo físico quando disponível. Se o usuário
// escolheu um perfil manual, ou o banco local estimou uma resolução, limita a
// quantização à resolução que a API de entrada realmente entrega: nunca inventa amostras.
static float AjustarPressaoPelosNiveis(float pressure, int sourceLevels) {
    pressure=std::max(0.0f,std::min(1.0f,pressure));
    int targetLevels=0;
    if(g_tabletManualPressureLevels>=2) targetLevels=g_tabletManualPressureLevels;
    else if(!g_wintabPressureAxisAvailable && g_tabletLevelsAreEstimated && g_tabletPressureLevelsDetected>=2)
        targetLevels=g_tabletPressureLevelsDetected;
    if(targetLevels>=2 && sourceLevels>=2) {
        const int usable=std::min(targetLevels,sourceLevels);
        if(usable>=2) pressure=(float)(std::floor((double)pressure*(usable-1)+0.5)/(double)(usable-1));
    }
    return std::max(0.0f,std::min(1.0f,pressure));
}

static void WintabForcarSolturaMouseGlobal() {
    if (g_wintabMouseInjectedDown) {
        INPUT in{}; in.type=INPUT_MOUSE; in.mi.dwFlags=MOUSEEVENTF_LEFTUP;
        SendInput(1,&in,sizeof(in));
    } else if (g_wintabExternalMouseActive && g_wintabFallbackMouseTarget && IsWindow(g_wintabFallbackMouseTarget)) {
        POINT q=g_wintabLastScreenPoint;
        ScreenToClient(g_wintabFallbackMouseTarget,&q);
        SendMessageW(g_wintabFallbackMouseTarget,WM_LBUTTONUP,0,MAKELPARAM((SHORT)q.x,(SHORT)q.y));
    }
    g_wintabMouseInjectedDown=false;
    g_wintabExternalMouseActive=false;
    g_wintabFallbackMouseTarget=NULL;
    g_wintabContactActive=false;
    g_wintabCanvasStrokeActive=false;
}

void FecharWintab() {
    WintabForcarSolturaMouseGlobal();
    if (g_wintabContext && g_wtClose) g_wtClose(g_wintabContext);
    g_wintabContext=NULL;
    g_wintabLastButtons=0; g_wintabButtonsInitialized=false;
    canetaPointerEmContato=false; desenhando=false;
    g_wintabHasLastClientPoint=false; g_wintabHasLastScreenPoint=false;
    g_wintabSpacePanHoverActive=false;
    RestaurarCursorSistemaWintab();
}

bool InicializarWintab(HWND hwnd) {
    if (g_wintabContext) return true;
    if (!g_wintabModule) g_wintabModule=LoadLibraryW(L"Wintab32.dll");
    if (!g_wintabModule) return false;
    if (!g_wtInfoA) g_wtInfoA=(KOD_WTInfoA)GetProcAddress(g_wintabModule,"WTInfoA");
    if (!g_wtOpenA) g_wtOpenA=(KOD_WTOpenA)GetProcAddress(g_wintabModule,"WTOpenA");
    if (!g_wtClose) g_wtClose=(KOD_WTClose)GetProcAddress(g_wintabModule,"WTClose");
    if (!g_wtPacket) g_wtPacket=(KOD_WTPacket)GetProcAddress(g_wintabModule,"WTPacket");
    if (!g_wtPacketsGet) g_wtPacketsGet=(KOD_WTPacketsGet)GetProcAddress(g_wintabModule,"WTPacketsGet");
    if (!g_wtGetA) g_wtGetA=(KOD_WTGetA)GetProcAddress(g_wintabModule,"WTGetA");
    if (!g_wtQueueSizeSet) g_wtQueueSizeSet=(KOD_WTQueueSizeSet)GetProcAddress(g_wintabModule,"WTQueueSizeSet");
    if (!g_wtQueueSizeGet) g_wtQueueSizeGet=(KOD_WTQueueSizeGet)GetProcAddress(g_wintabModule,"WTQueueSizeGet");
    if (!g_wtInfoA || !g_wtOpenA || !g_wtClose || (!g_wtPacket && !g_wtPacketsGet)) return false;

    KOD_LOGCONTEXTA lc{};
    if (!g_wtInfoA(KOD_WTI_DEFSYSCTX,0,&lc)) return false;
    // Contexto de sistema: o driver continua produzindo mouse global quando a
    // ponta está fora do canvas (outros programas, timeline e ferramentas).
    // No canvas, os pacotes Wintab continuam alimentando o traço com pressão.
    lc.lcOptions |= 0x0001u | KOD_CXO_MESSAGES | KOD_CXO_CSRMESSAGES;
    lc.lcSysMode = TRUE;
    lc.lcMsgBase=KOD_WT_DEFBASE;
    lc.lcPktData=KOD_PK_TIME|KOD_PK_CURSOR|KOD_PK_BUTTONS|KOD_PK_X|KOD_PK_Y|KOD_PK_NORMAL_PRESSURE;
    lc.lcPktMode=0; // valores absolutos
    lc.lcMoveMask=KOD_PK_CURSOR|KOD_PK_BUTTONS|KOD_PK_X|KOD_PK_Y|KOD_PK_NORMAL_PRESSURE;
    lc.lcBtnDnMask=0xFFFFFFFFu;
    lc.lcBtnUpMask=0xFFFFFFFFu;
    if (lc.lcInExtX == 0 || lc.lcInExtY == 0) return false;
    lc.lcOutOrgX=0; lc.lcOutExtX=lc.lcInExtX;
    lc.lcOutOrgY=0; lc.lcOutExtY=-lc.lcInExtY;

    g_wintabPressureMin=0; g_wintabPressureMax=1023; g_wintabPressureAxisAvailable=false;
    g_wintabLastButtons=0; g_wintabButtonsInitialized=false;
    g_wintabContext=g_wtOpenA(hwnd,&lc,TRUE);
    if (!g_wintabContext) return false;

    g_wintabContextConfig=lc;
    if (g_wtGetA) {
        KOD_LOGCONTEXTA actual{};
        if (g_wtGetA(g_wintabContext,&actual)) g_wintabContextConfig=actual;
    }
    // Krita consulta o eixo de pressão do dispositivo do contexto efetivamente aberto.
    KOD_WT_AXIS axis{};
    if (WintabLerEixoPressao(g_wintabContextConfig.lcDevice,&axis)) {
        g_wintabPressureMin=axis.axMin; g_wintabPressureMax=axis.axMax;
        g_wintabPressureAxisAvailable=(axis.axMax>axis.axMin);
        g_tabletWintabDeviceIndex=(int)g_wintabContextConfig.lcDevice;
    }
    // Alguns drivers de contexto de sistema deixam lcDevice genérico; neste caso
    // procuramos o primeiro dispositivo Wintab que exponha DVC_NPRESSURE.
    if(!g_wintabPressureAxisAvailable) {
        for(UINT dev=0;dev<16;++dev) {
            KOD_WT_AXIS candidate{};
            if(WintabLerEixoPressao(dev,&candidate)) {
                g_wintabPressureMin=candidate.axMin; g_wintabPressureMax=candidate.axMax;
                g_wintabPressureAxisAvailable=true; g_tabletWintabDeviceIndex=(int)dev; axis=candidate; break;
            }
        }
    }
    if(g_wintabPressureAxisAvailable) {
        long long levels=(long long)g_wintabPressureMax-(long long)g_wintabPressureMin+1LL;
        if(levels>=2 && levels<=65536) {
            g_tabletPressureLevelsDetected=(int)levels;
            g_tabletPressureLevelsSource=L"Eixo real de pressão (Wintab32.dll)";
            g_tabletLevelsAreEstimated=false;
        }
    }
    AtualizarPainelTablet();
    // O Krita usa fila 128; Wacom recomenda 32..128 para evitar overflow
    // sem pedir uma fila excessiva ao driver. Tenta tamanhos decrescentes.
    if (g_wtQueueSizeSet) {
        int oldSize=g_wtQueueSizeGet ? g_wtQueueSizeGet(g_wintabContext) : 0;
        if (oldSize != 128 && !g_wtQueueSizeSet(g_wintabContext,128)) {
            if (!g_wtQueueSizeSet(g_wintabContext,64))
                g_wtQueueSizeSet(g_wintabContext,32);
        }
    }
    AplicarCursorGlobalWintab();
    return true;
}
HMODULE g_user32Pointer = NULL;
void InicializarEntradaCanetaWindows() {
    if (!g_user32Pointer) g_user32Pointer = GetModuleHandleW(L"user32.dll");
    if (g_user32Pointer && !g_getPointerPenInfo)
        g_getPointerPenInfo = (KODANIMA_GetPointerPenInfo)GetProcAddress(g_user32Pointer, "GetPointerPenInfo");
    if (g_user32Pointer && !g_getPointerPenInfoHistory)
        g_getPointerPenInfoHistory = (KODANIMA_GetPointerPenInfoHistory)GetProcAddress(g_user32Pointer, "GetPointerPenInfoHistory");
}
float EspessuraPorPressao(float pressure) {
    // Só é chamada quando a mecânica Tamanho por Pressão está habilitada.
    // Não a bloqueie por pressaoCanetaAtiva: pincéis importados têm sua própria opção.
    pressure=std::max(0.0f,std::min(1.0f,pressure));
    // Curva convexa: pressões pequenas permanecem próximas da espessura mínima,
    // em vez de engrossarem cedo como acontecia com sqrt(pressão).
    const float curva=(float)std::pow((double)pressure,1.65);
    const float minWidth=std::max(0.01f,std::min(5000.0f,tamanhoMinimoPressao));
    const float maxWidth=std::max(minWidth,std::min(5000.0f,tamanhoMaximoPressao));
    return std::max(0.01f,minWidth+(maxWidth-minWidth)*curva);
}
wchar_t caminhoCursor[MAX_PATH] = L"";
HCURSOR cursorPersonalizado = NULL;
HWND hwndPopupFerramenta = NULL;
HWND hwndSettings = NULL;
HWND hwndShortcutPanel = NULL;
// No modo Wintab, o cursor escolhido pode ser aplicado ao cursor normal do sistema
// para continuar visível sobre janelas de outros programas. SPI_SETCURSORS restaura-o.
bool g_wintabCursorGlobalAtivo = false;
static void RestaurarCursorSistemaWintab() {
    if (g_wintabCursorGlobalAtivo) {
        SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, 0);
        g_wintabCursorGlobalAtivo = false;
    }
}
static HCURSOR CursorSelecionadoWintab() {
    return cursorPersonalizado ? cursorPersonalizado : LoadCursorW(NULL, MAKEINTRESOURCEW(32515));
}

static void AplicarCursorGlobalWintab() {
    if (pressureApi != 0) { RestaurarCursorSistemaWintab(); return; }
    HCURSOR c = NULL;
    if (cursorPersonalizado && caminhoCursor[0]) c=LoadCursorFromFileW(caminhoCursor);
    if (!c) {
        // SetSystemCursor toma posse do handle; copie a mira padrão para não
        // passar diretamente um cursor compartilhado retornado por LoadCursor.
        HCURSOR padrao=LoadCursorW(NULL,MAKEINTRESOURCEW(32515));
        if (padrao) c=(HCURSOR)CopyImage(padrao,IMAGE_CURSOR,0,0,LR_COPYFROMRESOURCE);
    }
    if (!c) return;
    if (SetSystemCursor(c, OCR_NORMAL)) g_wintabCursorGlobalAtivo = true;
    else DestroyCursor(c);
}

// WM_SETCURSOR também é enviado para controles-filhos (BUTTON, EDIT, etc.).
// O hook pós-procedimento impede que a classe de um botão troque a mira escolhida.
static bool CursorJanelaPertenceAoKodanima(HWND janela) {
    if (!janela || !IsWindow(janela)) return false;
    HWND raiz=GetAncestor(janela, GA_ROOTOWNER);
    if (!raiz) raiz=GetAncestor(janela, GA_ROOT);
    // Inclui canvas, timeline, ferramentas e a janela inicial. Dialogos de
    // preferencias/configuracao herdam o root-owner de uma destas janelas.
    return (raiz && (raiz==hwndMain || raiz==hwndCanvas || raiz==hwndTimeline || raiz==hwndTools)) ||
           janela==hwndMain || janela==hwndCanvas || janela==hwndTimeline || janela==hwndTools;
}
static LRESULT CALLBACK WintabCursorCallWndRetProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // O cursor escolhido deve sobreviver ao WM_SETCURSOR de BUTTON/EDIT/combos
    // nos dois modos de entrada. O hook so atua nas janelas do proprio Kodanima.
    if (nCode >= 0 && lParam) {
        const CWPRETSTRUCT* call=(const CWPRETSTRUCT*)lParam;
        if (call->message == WM_SETCURSOR && CursorJanelaPertenceAoKodanima(call->hwnd)) {
            HCURSOR wanted=CursorSelecionadoWintab();
            if (wanted && GetCursor()!=wanted) SetCursor(wanted);
        }
    }
    return CallNextHookEx(g_wintabCursorHook,nCode,wParam,lParam);
}
HWND hwndToolSettings = NULL;
wchar_t caminhoProjetoAtivo[MAX_PATH] = L"";
bool settingsMouseEntered = false;
bool toolSettingsMouseEntered = false;
HWND hwndEditTamanhoLapis = NULL;
bool lapisAntiAlias = true;
// Estabilizador global de traços do lápis/brush: 0 = desligado, 100 = máxima suavização.
bool estabilizadorTracoAtivo = true;
int nivelEstabilizadorTraco = 35;
int durezaPincel = 3; // 0 muito macio, 3 duro
struct BrushMetaKodanima {
    // Estrutura em memória. Os parâmetros são persistidos exclusivamente no JSON mestre.
    float size; float minSize; float maxSize; int hardness; int antialias;
    float spacingPercent; float angleDegrees; float roundnessPercent; float scatterPercent;
    float opacityPercent; float flowPercent; int rotateWithStroke; int sizeByPressure; int opacityByPressure;
    float startSizePercent; float endSizePercent; float startOpacityPercent; float endOpacityPercent;
    float sprayDensity; float sprayRadiusPercent; int particleCount; float particleSpreadPercent;
    float scaleRandomPercent; float blurPercent;
    int sprayingEffect; int randomRotation; int followPenDirection; int antiOverflow; int optimizeStroke;
    int startEndDynamics; int particlesEnabled;
};
struct BrushEntryKodanima {
    std::wstring name;
    std::wstring bmpPath;
    std::wstring texturePath;
    std::wstring texture2Path;
    std::wstring texture3Path;
    COLORREF texture2Tint=RGB(255,255,255), texture3Tint=RGB(70,70,70);
    float texture2Opacity=100.0f, texture3Opacity=100.0f;
    BrushMetaKodanima meta{};
};
static BrushMetaKodanima MetaPadraoImportacao();
static std::vector<BrushEntryKodanima> g_brushesKodanima;
static int g_brushSelecionado = -1;
static HBITMAP g_activeBrushBitmap = NULL;
static std::vector<unsigned char> g_activeBrushMask;
static int g_activeBrushWidth=0, g_activeBrushHeight=0;
static std::wstring g_activeBrushPath;
static std::vector<unsigned char> g_activeBrushTextureMask;
static int g_activeBrushTextureWidth=0, g_activeBrushTextureHeight=0;
static std::wstring g_activeBrushTexturePath;
static std::vector<unsigned char> g_activeBrushTextureMask2, g_activeBrushTextureMask3;
static int g_activeBrushTexture2Width=0,g_activeBrushTexture2Height=0,g_activeBrushTexture3Width=0,g_activeBrushTexture3Height=0;
static std::wstring g_activeBrushTexture2Path,g_activeBrushTexture3Path;
static COLORREF g_activeBrushTexture2Tint=RGB(255,255,255),g_activeBrushTexture3Tint=RGB(70,70,70);
static float g_activeBrushTexture2Opacity=100.0f,g_activeBrushTexture3Opacity=100.0f;
static bool g_brushOptimizedStroke=false;
struct BrushStrokeSegment { int x1,y1,x2,y2; float width; COLORREF color; };
static std::vector<BrushStrokeSegment> g_brushPendingSegments;
static std::wstring g_brushStorageError;
static DWORD g_brushStorageWin32Error=ERROR_SUCCESS;
static float brushSpacingPercent=18.0f;       // distância entre carimbos, em % do diâmetro
static float brushAngleDegrees=0.0f;          // ângulo fixo da ponta
static float brushRoundnessPercent=100.0f;    // 100=circular/original; menor=achatado
static float brushScatterPercent=0.0f;        // dispersão lateral em % do diâmetro
static float brushOpacityPercent=100.0f;
static float brushFlowPercent=100.0f;
static bool brushRotateWithStroke=true;
static bool brushSizeByPressure=true;
static bool brushOpacityByPressure=false;
static HWND hwndBrushManager = NULL;
static HWND hwndBrushProperties = NULL;
static bool g_useImportedBrushes = true;
static int g_brushPropertiesPage = 0;
static int g_brushPropertiesReturnPage = 0;
static HWND g_brushPropertiesOwner = NULL;
static bool g_restoreToolsAfterMin=false,g_restoreTimelineAfterMin=false,g_restoreBrushManagerAfterMin=false,g_restoreToolSettingsAfterMin=false,g_restoreBrushPropertiesAfterMin=false,g_restoreShortcutsAfterMin=false;
HWND hwndEditGap = NULL;
HWND hwndEditToleranciaBalde = NULL;
HWND hwndComboTema = NULL;
HWND hwndComboRenderer = NULL;
HWND hwndComboUIScale = NULL;
HWND hwndComboTextSize = NULL;
HWND hwndComboCores = NULL;
HWND hwndComboRAM = NULL;
int nucleosDesejados = 0; // 0 = todos os núcleos disponíveis
int limiteRAMMB = 0; // 0 = sem limite configurado
HANDLE hJobLimiteMemoria = NULL;
DWORD_PTR g_mascaraAfinidadeOriginal = 0;
IDirect3D9* g_d3d9 = NULL;
IDirect3DDevice9* g_d3dDevice9 = NULL;
IDirect3DTexture9* g_dx9CanvasTexture = NULL;
int g_dx9TextureWidth = 0, g_dx9TextureHeight = 0;
bool g_dx9FalhaAvisada = false;
// Motores DX11/DX12 carregados dinamicamente para preservar a inicialização em XP.
HMODULE g_modD3D11=NULL, g_modDXGI=NULL, g_modD3D12=NULL;
ID3D11Device* g_d3d11Device=NULL; ID3D11DeviceContext* g_d3d11Context=NULL;
IDXGISwapChain* g_dx11Swap=NULL; ID3D11Texture2D* g_dx11Texture=NULL;
int g_dx11W=0,g_dx11H=0; bool g_dx11FalhaAvisada=false;
ID3D12Device* g_d3d12Device=NULL; ID3D12CommandQueue* g_d3d12Queue=NULL;
IDXGISwapChain3* g_dx12Swap=NULL; ID3D12CommandAllocator* g_dx12Allocator=NULL;
ID3D12GraphicsCommandList* g_dx12List=NULL; ID3D12Fence* g_dx12Fence=NULL;
ID3D12Resource* g_dx12Upload=NULL; UINT64 g_dx12UploadSize=0, g_dx12FenceValue=0;
HANDLE g_dx12FenceEvent=NULL; int g_dx12W=0,g_dx12H=0; bool g_dx12FalhaAvisada=false;
int rendererPreferido = 0; // 0=GDI, 1=DirectX 9, 2=DirectX 11, 3=DirectX 12
struct KodaCPUFeatures {
    bool mmx=false, sse=false, sse2=false, sse3=false, ssse3=false;
    bool sse41=false, sse42=false, aes=false, vmx=false, svm=false;
    bool avx=false, avx2=false, osxsave=false, avxUsable=false, avx2Usable=false;
    int pixelKernel=0; // 0=escalar, 1=SSE2, 2=AVX2
    int detectedCount=0;
    wchar_t vendor[32]=L"Desconhecido";
    wchar_t brand[64]=L"CPU x86 não identificada";
};
static KodaCPUFeatures g_kodaCPU;
int escalaInterface = 100;
int escalaAplicada = 100;
int tamanhoTexto = 100;
HFONT fontePreferencias = NULL;
HWND hwndCheckPressao = NULL;

int frameAtual = 0;
bool onionSkinAtivo = false;
int opacidadeOnion = 40;
// Backbuffer persistente do canvas: evita alocar bitmap/DC a cada WM_PAINT.
HDC g_canvasBufferDC = NULL;
HBITMAP g_canvasBufferBitmap = NULL;
HBITMAP g_canvasBufferOldBitmap = NULL;
int g_canvasBufferWidth = 0;
int g_canvasBufferHeight = 0;
HBITMAP g_canvasCompositeCache = NULL;
int g_canvasCompositeCacheFrame = -1;
bool g_canvasCompositeCacheDirty = true;
void InvalidarCacheComposicaoCanvas();
HBITMAP ObterQuadroCompostoCache(int idx);

// Detecta capacidades com CPUID em tempo de execução. AVX/AVX2 só são
// considerados utilizáveis quando o SO habilita XMM/YMM via OSXSAVE/XCR0.
static void DetectarRecursosCPU() {
    g_kodaCPU = KodaCPUFeatures{};
#if KODA_GNU_X86_SIMD
    unsigned int eax=0, ebx=0, ecx=0, edx=0;
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
        char vendor[13]={0};
        memcpy(vendor, &ebx, 4); memcpy(vendor+4, &edx, 4); memcpy(vendor+8, &ecx, 4);
        for (int i=0; i<12 && vendor[i]; ++i) g_kodaCPU.vendor[i]=(wchar_t)(unsigned char)vendor[i];
        g_kodaCPU.vendor[12]=0;
        unsigned int maxBasic=eax;
        if (maxBasic >= 1 && __get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            g_kodaCPU.mmx=(edx & (1u<<23))!=0;
            g_kodaCPU.sse=(edx & (1u<<25))!=0;
            g_kodaCPU.sse2=(edx & (1u<<26))!=0;
            g_kodaCPU.sse3=(ecx & (1u<<0))!=0;
            g_kodaCPU.ssse3=(ecx & (1u<<9))!=0;
            g_kodaCPU.sse41=(ecx & (1u<<19))!=0;
            g_kodaCPU.sse42=(ecx & (1u<<20))!=0;
            g_kodaCPU.aes=(ecx & (1u<<25))!=0;
            g_kodaCPU.vmx=(ecx & (1u<<5))!=0; // Intel VMX / VT-x
            g_kodaCPU.osxsave=(ecx & (1u<<27))!=0;
            g_kodaCPU.avx=(ecx & (1u<<28))!=0;
        }
        unsigned int maxExt=0;
        if (__get_cpuid(0x80000000u, &eax, &ebx, &ecx, &edx)) maxExt=eax;
        if (maxExt >= 0x80000001u && __get_cpuid(0x80000001u, &eax, &ebx, &ecx, &edx))
            g_kodaCPU.svm=(ecx & (1u<<2))!=0; // AMD-V
        if (maxExt >= 0x80000004u) {
            unsigned int regs[12]={0};
            __get_cpuid(0x80000002u,&regs[0],&regs[1],&regs[2],&regs[3]);
            __get_cpuid(0x80000003u,&regs[4],&regs[5],&regs[6],&regs[7]);
            __get_cpuid(0x80000004u,&regs[8],&regs[9],&regs[10],&regs[11]);
            char brand[49]={0}; memcpy(brand,regs,48);
            int j=0; while(j<48 && brand[j]==' ') ++j;
            // Limpa a string padrão antes de copiar a marca para não deixar o
            // sufixo "CPU x86 não identificada" concatenado ao modelo detectado.
            memset(g_kodaCPU.brand,0,sizeof(g_kodaCPU.brand));
            for(int i=0; j<48 && brand[j] && i<63; ++i,++j) g_kodaCPU.brand[i]=(wchar_t)(unsigned char)brand[j];
            g_kodaCPU.brand[63]=0;
            if (g_kodaCPU.brand[0]==0) lstrcpynW(g_kodaCPU.brand,L"CPU x86 identificada",64);
        }
        if (g_kodaCPU.avx && g_kodaCPU.osxsave) {
            unsigned int xcrLow=0, xcrHigh=0;
            // XGETBV só é executado após confirmar AVX + OSXSAVE.
            __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(xcrLow), "=d"(xcrHigh) : "c"(0));
            const unsigned long long xcr0=((unsigned long long)xcrHigh<<32)|xcrLow;
            g_kodaCPU.avxUsable=(xcr0 & 0x6ULL)==0x6ULL;
        }
        if (maxBasic >= 7) {
            unsigned int maxLeaf=0;
            __get_cpuid(0,&maxLeaf,&ebx,&ecx,&edx);
            if (maxLeaf >= 7) {
                __get_cpuid_count(7,0,&eax,&ebx,&ecx,&edx);
                g_kodaCPU.avx2=(ebx & (1u<<5))!=0;
            }
        }
        g_kodaCPU.avx2Usable=g_kodaCPU.avx2 && g_kodaCPU.avxUsable;
    }
#endif
    if (g_kodaCPU.avx2Usable) g_kodaCPU.pixelKernel=2;
    else if (g_kodaCPU.sse2) g_kodaCPU.pixelKernel=1;
    else g_kodaCPU.pixelKernel=0;
    const bool flags[]={g_kodaCPU.mmx,g_kodaCPU.sse,g_kodaCPU.sse2,g_kodaCPU.sse3,
        g_kodaCPU.ssse3,g_kodaCPU.sse41,g_kodaCPU.sse42,g_kodaCPU.avx,g_kodaCPU.avx2,
        g_kodaCPU.aes,(g_kodaCPU.vmx||g_kodaCPU.svm)};
    for(size_t i=0;i<sizeof(flags)/sizeof(flags[0]);++i) if(flags[i]) ++g_kodaCPU.detectedCount;
}

#if KODA_GNU_X86_SIMD
static __attribute__((target("sse2"))) void KodaConverterLinhaSSE2(const BYTE* src, BYTE* dst, int count, bool whiteTransparent) {
    const __m128i channelMask=_mm_set1_epi32(255);
    const __m128i alphaMask=_mm_set1_epi32((int)0xFF000000u);
    const __m128i threshold=_mm_set1_epi32(249);
    int x=0;
    for(;x+4<=count;x+=4) {
        __m128i v=_mm_loadu_si128((const __m128i*)(src+(size_t)x*4));
        __m128i b=_mm_and_si128(v,channelMask);
        __m128i g=_mm_and_si128(_mm_srli_epi32(v,8),channelMask);
        __m128i r=_mm_and_si128(_mm_srli_epi32(v,16),channelMask);
        __m128i a=alphaMask;
        if(whiteTransparent) {
            __m128i white=_mm_and_si128(_mm_cmpgt_epi32(r,threshold),
                _mm_and_si128(_mm_cmpgt_epi32(g,threshold),_mm_cmpgt_epi32(b,threshold)));
            a=_mm_andnot_si128(white,alphaMask);
        }
        __m128i out=_mm_or_si128(r,_mm_or_si128(_mm_slli_epi32(g,8),
            _mm_or_si128(_mm_slli_epi32(b,16),a)));
        _mm_storeu_si128((__m128i*)(dst+(size_t)x*4),out);
    }
    for(;x<count;++x) {
        BYTE b=src[x*4],g=src[x*4+1],r=src[x*4+2];
        dst[x*4]=r; dst[x*4+1]=g; dst[x*4+2]=b;
        dst[x*4+3]=(whiteTransparent&&r>=250&&g>=250&&b>=250)?0:255;
    }
}
static __attribute__((target("avx2"))) void KodaConverterLinhaAVX2(const BYTE* src, BYTE* dst, int count, bool whiteTransparent) {
    const __m256i channelMask=_mm256_set1_epi32(255);
    const __m256i alphaMask=_mm256_set1_epi32((int)0xFF000000u);
    const __m256i threshold=_mm256_set1_epi32(249);
    int x=0;
    for(;x+8<=count;x+=8) {
        __m256i v=_mm256_loadu_si256((const __m256i*)(src+(size_t)x*4));
        __m256i b=_mm256_and_si256(v,channelMask);
        __m256i g=_mm256_and_si256(_mm256_srli_epi32(v,8),channelMask);
        __m256i r=_mm256_and_si256(_mm256_srli_epi32(v,16),channelMask);
        __m256i a=alphaMask;
        if(whiteTransparent) {
            __m256i white=_mm256_and_si256(_mm256_cmpgt_epi32(r,threshold),
                _mm256_and_si256(_mm256_cmpgt_epi32(g,threshold),_mm256_cmpgt_epi32(b,threshold)));
            a=_mm256_andnot_si256(white,alphaMask);
        }
        __m256i out=_mm256_or_si256(r,_mm256_or_si256(_mm256_slli_epi32(g,8),
            _mm256_or_si256(_mm256_slli_epi32(b,16),a)));
        _mm256_storeu_si256((__m256i*)(dst+(size_t)x*4),out);
    }
    for(;x<count;++x) {
        BYTE b=src[x*4],g=src[x*4+1],r=src[x*4+2];
        dst[x*4]=r; dst[x*4+1]=g; dst[x*4+2]=b;
        dst[x*4+3]=(whiteTransparent&&r>=250&&g>=250&&b>=250)?0:255;
    }
}
#endif

// DIB temporário persistente compartilhado pelos caminhos DirectX; evita criar
// HBITMAP/HDC/GetDC em toda repintura do canvas.
static HDC g_canvasStagingDC=NULL;
static HBITMAP g_canvasStagingBitmap=NULL;
static HBITMAP g_canvasStagingOldBitmap=NULL;
static BYTE* g_canvasStagingBits=NULL;
static int g_canvasStagingW=0, g_canvasStagingH=0;
// Scratch DIB reutilizável para composição de traços suavizados sem GetPixel/SetPixel.
static HDC g_brushBlendScratchDC=NULL;
static HBITMAP g_brushBlendScratchBitmap=NULL;
static HBITMAP g_brushBlendScratchOldBitmap=NULL;
static BYTE* g_brushBlendScratchBits=NULL;
static int g_brushBlendScratchW=0, g_brushBlendScratchH=0;
static void DestruirScratchPincel() {
    if(g_brushBlendScratchDC && g_brushBlendScratchOldBitmap)
        SelectObject(g_brushBlendScratchDC,g_brushBlendScratchOldBitmap);
    if(g_brushBlendScratchBitmap) DeleteObject(g_brushBlendScratchBitmap);
    if(g_brushBlendScratchDC) DeleteDC(g_brushBlendScratchDC);
    g_brushBlendScratchDC=NULL; g_brushBlendScratchBitmap=NULL; g_brushBlendScratchOldBitmap=NULL;
    g_brushBlendScratchBits=NULL; g_brushBlendScratchW=g_brushBlendScratchH=0;
}
static bool GarantirScratchPincel(int w,int h) {
    if(w<=0||h<=0) return false;
    if(g_brushBlendScratchDC&&g_brushBlendScratchBitmap&&g_brushBlendScratchBits&&
       g_brushBlendScratchW>=w&&g_brushBlendScratchH>=h) return true;
    // Mantém capacidade extra para os próximos segmentos e evita realocar em toda pincelada.
    int nw=std::max(w,g_brushBlendScratchW), nh=std::max(h,g_brushBlendScratchH);
    if(nw<w) nw=w; if(nh<h) nh=h;
    if(nw<4096) nw=std::max(w,nw*2); else nw=w;
    if(nh<512) nh=std::max(h,nh*2); else nh=h;
    const unsigned long long bytes=(unsigned long long)(unsigned int)nw*(unsigned long long)(unsigned int)nh*4ULL;
    if(bytes>64ULL*1024ULL*1024ULL) { nw=w; nh=h; }
    DestruirScratchPincel();
    HDC screen=GetDC(NULL); if(!screen) return false;
    BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=nw;
    bi.bmiHeader.biHeight=-nh; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
    g_brushBlendScratchBitmap=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,(void**)&g_brushBlendScratchBits,NULL,0);
    g_brushBlendScratchDC=CreateCompatibleDC(screen); ReleaseDC(NULL,screen);
    if(!g_brushBlendScratchBitmap||!g_brushBlendScratchDC||!g_brushBlendScratchBits) { DestruirScratchPincel(); return false; }
    g_brushBlendScratchOldBitmap=(HBITMAP)SelectObject(g_brushBlendScratchDC,g_brushBlendScratchBitmap);
    if(!g_brushBlendScratchOldBitmap||g_brushBlendScratchOldBitmap==(HBITMAP)HGDI_ERROR) {
        g_brushBlendScratchOldBitmap=NULL; DestruirScratchPincel(); return false;
    }
    g_brushBlendScratchW=nw; g_brushBlendScratchH=nh; return true;
}
static void DestruirCanvasStaging() {
    if(g_canvasStagingDC && g_canvasStagingOldBitmap)
        SelectObject(g_canvasStagingDC,g_canvasStagingOldBitmap);
    if(g_canvasStagingBitmap) DeleteObject(g_canvasStagingBitmap);
    if(g_canvasStagingDC) DeleteDC(g_canvasStagingDC);
    g_canvasStagingDC=NULL; g_canvasStagingBitmap=NULL; g_canvasStagingOldBitmap=NULL;
    g_canvasStagingBits=NULL; g_canvasStagingW=g_canvasStagingH=0;
}
static bool GarantirCanvasStaging(int w,int h) {
    if(w<=0||h<=0) return false;
    if(g_canvasStagingDC&&g_canvasStagingBitmap&&g_canvasStagingBits&&g_canvasStagingW==w&&g_canvasStagingH==h) return true;
    DestruirCanvasStaging();
    HDC screen=GetDC(NULL); if(!screen) return false;
    BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=w;
    bi.bmiHeader.biHeight=-h; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
    g_canvasStagingBitmap=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,(void**)&g_canvasStagingBits,NULL,0);
    g_canvasStagingDC=CreateCompatibleDC(screen);
    ReleaseDC(NULL,screen);
    if(!g_canvasStagingBitmap||!g_canvasStagingDC||!g_canvasStagingBits) { DestruirCanvasStaging(); return false; }
    g_canvasStagingOldBitmap=(HBITMAP)SelectObject(g_canvasStagingDC,g_canvasStagingBitmap);
    if(!g_canvasStagingOldBitmap||g_canvasStagingOldBitmap==(HBITMAP)HGDI_ERROR) { g_canvasStagingOldBitmap=NULL; DestruirCanvasStaging(); return false; }
    g_canvasStagingW=w; g_canvasStagingH=h; return true;
}
static bool CopiarCanvasParaStaging(int w,int h) {
    return g_canvasBufferDC && GarantirCanvasStaging(w,h) &&
        BitBlt(g_canvasStagingDC,0,0,w,h,g_canvasBufferDC,0,0,SRCCOPY)!=FALSE;
}

void InvalidarCacheComposicaoCanvas() {
    // Não apagar a última imagem válida no momento da edição. Apenas marca-a
    // como suja; a nova composição substitui a antiga quando estiver pronta.
    g_canvasCompositeCacheDirty = true;
}

void DestruirBackbufferCanvas() {
    DestruirCanvasStaging();
    DestruirScratchPincel();
    if (g_canvasBufferDC) {
        if (g_canvasBufferOldBitmap) SelectObject(g_canvasBufferDC, g_canvasBufferOldBitmap);
        if (g_canvasBufferBitmap) DeleteObject(g_canvasBufferBitmap);
        DeleteDC(g_canvasBufferDC);
    }
    g_canvasBufferDC = NULL;
    g_canvasBufferBitmap = NULL;
    g_canvasBufferOldBitmap = NULL;
    g_canvasBufferWidth = g_canvasBufferHeight = 0;
}

bool GarantirBackbufferCanvas(HDC referencia, int w, int h) {
    w = std::max(1, w); h = std::max(1, h);
    if (g_canvasBufferDC && g_canvasBufferBitmap && g_canvasBufferWidth == w && g_canvasBufferHeight == h) return true;
    DestruirBackbufferCanvas();
    g_canvasBufferDC = CreateCompatibleDC(referencia);
    if (!g_canvasBufferDC) return false;
    g_canvasBufferBitmap = CreateCompatibleBitmap(referencia, w, h);
    if (!g_canvasBufferBitmap) { DestruirBackbufferCanvas(); return false; }
    g_canvasBufferOldBitmap = (HBITMAP)SelectObject(g_canvasBufferDC, g_canvasBufferBitmap);
    if (!g_canvasBufferOldBitmap || g_canvasBufferOldBitmap == (HBITMAP)HGDI_ERROR) {
        g_canvasBufferOldBitmap = NULL; DestruirBackbufferCanvas(); return false;
    }
    g_canvasBufferWidth = w; g_canvasBufferHeight = h;
    return true;
}
char ferramentaAtual = 'L'; // 'L'=Lápis, 'E'=Borracha, 'B'=Balde, 'A'=Laço, 'M'=Mão, 'I'=conta-gotas
char ferramentaAnterior = 'L';
COLORREF corAtual = RGB(0, 0, 0);

float zoomFactor = 1.0f;
int panX = 0;
int panY = 0;
// Atalho sequencial P e depois 0: reenquadra a folha sem alterar zoom/rotacao.
DWORD g_atalhoCentralizarPPressionadoEm = 0;
bool desenhando = false;
bool navegandoCanvas = false;
bool g_spacePanModifierDown = false; // teclado global; funciona mesmo com foco fora do HWND do canvas
static bool EspacoParaMoverAtivo() {
    return g_spacePanModifierDown || ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0);
}
bool rotacionandoCanvas = false;
bool canvasSomente = false;
bool previewAtivo = false;
DWORD ultimaAcaoAtalho = 0;
POINT ultimoMouse;
float anguloCanvas = 0.0f;
RECT rectLaco = {0, 0, 0, 0};
bool lacoAtivo = false;
// Preferências independentes da borracha e da seleção por laço.
int tamanhoBorracha = 12;
bool pressaoBorrachaAtiva = true;
int pressaoMinBorracha = 2, pressaoMaxBorracha = 36;
int modoLaco = 1; // padrão: laço livre; 0 = retângulo, 1 = seleção livre desenhada
COLORREF corSelecao = RGB(40,150,255);
bool criandoSelecao = false;
POINT inicioSelecaoTela = {0,0}, ultimoPontoLacoTela = {0,0};
std::vector<POINT> pontosLacoTela;
// O caminho do laço também é guardado em coordenadas do documento. Assim,
// seleção/corte continuam corretos mesmo se a vista for movida ou rotacionada.
std::vector<POINT> pontosLacoBitmap;
RECT rectSelecaoTela = {0,0,0,0};
bool selecaoAtiva = false;
int operacaoSelecao = 0; // 0 nenhuma, 1 transformar, 2 copiar, 3 recortar
// Seleção raster real (BMP), transformações e clipboard interno entre quadros/camadas.
HBITMAP bitmapSelecaoRaster = NULL;
HBITMAP bitmapClipboardSelecao = NULL;
HBITMAP bitmapBackupTransformacao = NULL;
HBITMAP bitmapPreviewColagemVermelho = NULL;
// Cache transitório em RAM só do objeto selecionado, nunca do canvas inteiro.
// É recalculado apenas quando o ângulo/escala ou a seleção realmente mudam.
HBITMAP bitmapCacheRotacaoSelecao = NULL;
HBITMAP bitmapCacheFonteSelecao = NULL;
int bitmapCacheRotacaoX=0, bitmapCacheRotacaoY=0;
int bitmapCacheRotacaoW=0, bitmapCacheRotacaoH=0;
double bitmapCacheRotacaoAngulo=1.0e30;
float bitmapCacheRotacaoEscala=-1.0f;
// O backup de cancelamento também armazena somente o retângulo selecionado.
int backupTransformacaoX=0, backupTransformacaoY=0;
int backupTransformacaoW=0, backupTransformacaoH=0;
int selecaoBmpX=0, selecaoBmpY=0, selecaoBmpW=0, selecaoBmpH=0;
bool transformacaoSelecaoAtiva=false, colandoSelecao=false, transformacaoRotacao=false, transformacaoEscalaForcada=false;
bool transformacaoMoverSelecao=false;
double selecaoMoverRestoX=0.0, selecaoMoverRestoY=0.0;
bool colagemModoRotacao=false, colagemModoEscala=false;
float colagemEscala=1.0f; double colagemAngulo=0.0; POINT colagemPontoTransformacao={0,0};
int dpiProjeto=96, ultimoProjetoW=800, ultimoProjetoH=600, ultimoProjetoFPS=24;
int ferramentaSelecionadaID=ID_TOOL_LAPIS;
bool timelineSegueTema=true; COLORREF corTimelinePersonalizada=RGB(42,46,54);
float escalaSelecaoRaster=1.0f;
double anguloSelecaoRaster=0.0;
POINT centroSelecaoBmp={0,0}, cursorColagemBmp={0,0};
POINT cursorTransformacao={0,0};


// Limites do Cache (1MB a 200MB)
const size_t CACHE_MIN_BYTES = 1 * 1024 * 1024;
const size_t CACHE_MAX_BYTES = 200 * 1024 * 1024;

// Protótipos
LRESULT CALLBACK WndProcMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcCanvas(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void KodaVerificarBordaBarraTarefasUmaVez();
LRESULT CALLBACK WndProcTimeline(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcTools(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcSettings(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool SelecionarPasta(HWND hwndOwner, wchar_t* outPath);
HBITMAP CriarBitmapEmBranco(int w, int h, COLORREF corFundo);
void SalvarProjetoCompleto();
bool CarregarProjetoCompleto(const wchar_t* caminhoPastaRaiz);
static void KodaImportarArquivo(HWND owner, const wchar_t* path);
static void KodaMostrarDialogoImportar(HWND owner);
static void KodaColarImagemClipboard(HWND owner);
static void KodaProcessarDrop(HWND hwndDestino, HDROP drop);
bool SalvarHBITMAPComoBMP(HBITMAP hbmp, const wchar_t* filename);
void SalvarEstadoHistorico(FrameData& frame);
void ExecutarUndo();
void ExecutarRedo();
void AbrirJanelasModulares();
bool RenderizarMP4(HWND hwndOwner);
void IrParaQuadro(int indice);
void AlternarCanvasMaximizado();
void FecharTodasJanelas();
HBITMAP CriarBitmapOnion(HBITMAP origem, COLORREF corTint);
void DesenharBitmapCanvas(HDC hdc, HBITMAP bitmap, HWND hwnd, bool onion, COLORREF corTint);
void AlternarPreview();
void InserirQuadroNoMeio();
void AbrirPreferenciasFerramenta(HWND owner, int toolId=-1);
void AplicarEscalaBotoesTimeline();
static BOOL CALLBACK InstalarSubclassFocoHover(HWND child, LPARAM param);
static BOOL CALLBACK CapturarGeometriaBaseTimeline(HWND child, LPARAM param);
static BOOL CALLBACK EscalarControleTimeline(HWND child, LPARAM param);
static void AplicarEscalaFerramentas();
static void ReativarTimerFocoAutomatico();
static void AtualizarCampoDuracao(HWND hwnd);
void AplicarDeletePelaPosicaoDoMouse();
void LimparDesenhoDoQuadroAtual();
void ExcluirQuadroAtual();
void GarantirBitmapsCamadas(FrameData& f);
void SincronizarAliasCamadaAtiva();
void ExcluirBitmapsQuadro(FrameData& f);
HBITMAP ComporQuadroComCamadas(int idx);
HBITMAP ComporQuadroNoTempo(double segundos);
static double DuracaoCamadaQuadro(const FrameData& f, int layer);
static bool QuadroExisteNaCamada(const FrameData& f, int layer);
static int CriarQuadroNaCamada(int layer, int insertAfter);
static double TimelineSecondsTotalExact();
void AplicarBaldeComTolerancia(HBITMAP bitmap, int x, int y);
LRESULT CALLBACK WndProcToolSettings(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubclassBotaoFerramenta(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData);
LRESULT CALLBACK SubclassCampoDuracao(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData);
LRESULT CALLBACK SubclassCampoFPS(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData);

// -----------------------------------------------------------------------------
// Idiomas e localização Win32. O seletor contém mais de 80 idiomas. Os textos
// catalogados são traduzidos automaticamente; strings sem entrada no catálogo
// permanecem no idioma original, permitindo ampliar os pacotes sem quebrar a UI.
// -----------------------------------------------------------------------------
void SalvarConfiguracoesUsuario();
LRESULT CALLBACK WndProcLanguage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void AbrirSeletorIdioma(HWND owner);

struct KodaLanguageItem { const wchar_t* code; const wchar_t* nativeName; };
static const KodaLanguageItem g_kodaLanguages[] = {
    {L"pt-BR",L"Português (Brasil)"},{L"pt-PT",L"Português (Portugal)"},
    {L"en",L"English"},{L"es",L"Español"},{L"fr",L"Français"},{L"de",L"Deutsch"},
    {L"it",L"Italiano"},{L"nl",L"Nederlands"},{L"sv",L"Svenska"},{L"no",L"Norsk Bokmål"},
    {L"da",L"Dansk"},{L"fi",L"Suomi"},{L"is",L"Íslenska"},{L"ga",L"Gaeilge"},
    {L"cy",L"Cymraeg"},{L"ca",L"Català"},{L"gl",L"Galego"},{L"eu",L"Euskara"},
    {L"ru",L"Русский"},{L"uk",L"Українська"},{L"be",L"Беларуская"},{L"pl",L"Polski"},
    {L"cs",L"Čeština"},{L"sk",L"Slovenčina"},{L"sl",L"Slovenščina"},{L"hr",L"Hrvatski"},
    {L"sr",L"Српски"},{L"bs",L"Bosanski"},{L"bg",L"Български"},{L"ro",L"Română"},
    {L"hu",L"Magyar"},{L"el",L"Ελληνικά"},{L"tr",L"Türkçe"},{L"az",L"Azərbaycanca"},
    {L"kk",L"Қазақша"},{L"uz",L"O‘zbekcha"},{L"ky",L"Кыргызча"},{L"hy",L"Հայերեն"},
    {L"ka",L"ქართული"},{L"et",L"Eesti"},{L"lv",L"Latviešu"},{L"lt",L"Lietuvių"},
    {L"sq",L"Shqip"},{L"mk",L"Македонски"},{L"ar",L"العربية"},{L"he",L"עברית"},
    {L"fa",L"فارسی"},{L"ur",L"اردو"},{L"ps",L"پښتو"},{L"hi",L"हिन्दी"},
    {L"bn",L"বাংলা"},{L"pa",L"ਪੰਜਾਬੀ"},{L"gu",L"ગુજરાતી"},{L"mr",L"मराठी"},
    {L"ne",L"नेपाली"},{L"si",L"සිංහල"},{L"ta",L"தமிழ்"},{L"te",L"తెలుగు"},
    {L"kn",L"ಕನ್ನಡ"},{L"ml",L"മലയാളം"},{L"or",L"ଓଡ଼ିଆ"},{L"as",L"অসমীয়া"},
    {L"zh-CN",L"简体中文"},{L"zh-TW",L"繁體中文"},{L"ja",L"日本語"},{L"ko",L"한국어"},
    {L"th",L"ไทย"},{L"vi",L"Tiếng Việt"},{L"id",L"Bahasa Indonesia"},{L"ms",L"Bahasa Melayu"},
    {L"tl",L"Filipino"},{L"sw",L"Kiswahili"},{L"am",L"አማርኛ"},{L"yo",L"Yorùbá"},
    {L"ig",L"Igbo"},{L"ha",L"Hausa"},{L"zu",L"isiZulu"},{L"xh",L"isiXhosa"},
    {L"af",L"Afrikaans"},{L"so",L"Soomaali"},{L"mg",L"Malagasy"},{L"km",L"ភាសាខ្មែរ"},
    {L"my",L"မြန်မာ"},{L"lo",L"ລາວ"},{L"mn",L"Монгол"},{L"bo",L"བོད་སྐད་"},
    {L"mi",L"Te Reo Māori"},{L"eo",L"Esperanto"},{L"la",L"Latina"},{L"jv",L"Basa Jawa"},
    {L"su",L"Basa Sunda"},{L"ceb",L"Cebuano"},{L"war",L"Waray"},{L"ilo",L"Ilokano"}
};

static std::wstring KodaIdiomaBase() {
    size_t p=g_languageCode.find(L'-');
    return p==std::wstring::npos?g_languageCode:g_languageCode.substr(0,p);
}

// Catálogo integrado para os rótulos mais frequentes. A tradução cobre os
// elementos comuns da tela inicial, configuração do projeto e propriedades.
static const std::map<std::wstring,std::map<std::wstring,std::wstring> >& KodaCatalogoTraducao() {
    static const std::map<std::wstring,std::map<std::wstring,std::wstring> > t = {
        {L"Language", {{L"en",L"Language"},{L"es",L"Idioma"},{L"fr",L"Langue"},{L"de",L"Sprache"},{L"it",L"Lingua"},{L"ru",L"Язык"},{L"zh",L"语言"},{L"ja",L"言語"},{L"ko",L"언어"},{L"ar",L"اللغة"},{L"hi",L"भाषा"},{L"id",L"Bahasa"},{L"tr",L"Dil"},{L"pl",L"Język"},{L"nl",L"Taal"}}},
        {L"Language settings", {{L"en",L"Language settings"},{L"es",L"Configuración de idioma"},{L"fr",L"Paramètres de langue"},{L"de",L"Spracheinstellungen"},{L"it",L"Impostazioni lingua"},{L"ru",L"Настройки языка"},{L"zh",L"语言设置"},{L"ja",L"言語設定"},{L"ko",L"언어 설정"},{L"ar",L"إعدادات اللغة"},{L"hi",L"भाषा सेटिंग्स"},{L"id",L"Pengaturan bahasa"},{L"tr",L"Dil ayarları"},{L"pl",L"Ustawienia języka"},{L"nl",L"Taalinstellingen"}}},
        {L"Choose the interface language:", {{L"en",L"Choose the interface language:"},{L"es",L"Elige el idioma de la interfaz:"},{L"fr",L"Choisissez la langue de l’interface :"},{L"de",L"Sprache der Oberfläche auswählen:"},{L"it",L"Scegli la lingua dell’interfaccia:"},{L"ru",L"Выберите язык интерфейса:"},{L"zh",L"选择界面语言："},{L"ja",L"インターフェースの言語を選択："},{L"ko",L"인터페이스 언어 선택:"},{L"ar",L"اختر لغة الواجهة:"},{L"hi",L"इंटरफ़ेस भाषा चुनें:"},{L"id",L"Pilih bahasa antarmuka:"},{L"tr",L"Arayüz dilini seçin:"},{L"pl",L"Wybierz język interfejsu:"},{L"nl",L"Kies de interfacetaal:"}}},
        {L"Apply", {{L"en",L"Apply"},{L"es",L"Aplicar"},{L"fr",L"Appliquer"},{L"de",L"Übernehmen"},{L"it",L"Applica"},{L"ru",L"Применить"},{L"zh",L"应用"},{L"ja",L"適用"},{L"ko",L"적용"},{L"ar",L"تطبيق"},{L"hi",L"लागू करें"},{L"id",L"Terapkan"},{L"tr",L"Uygula"},{L"pl",L"Zastosuj"},{L"nl",L"Toepassen"}}},
        {L"Cancel", {{L"en",L"Cancel"},{L"es",L"Cancelar"},{L"fr",L"Annuler"},{L"de",L"Abbrechen"},{L"it",L"Annulla"},{L"ru",L"Отмена"},{L"zh",L"取消"},{L"ja",L"キャンセル"},{L"ko",L"취소"},{L"ar",L"إلغاء"},{L"hi",L"रद्द करें"},{L"id",L"Batal"},{L"tr",L"İptal"},{L"pl",L"Anuluj"},{L"nl",L"Annuleren"}}},
        {L"Cancelar", {{L"en",L"Cancel"},{L"es",L"Cancelar"},{L"fr",L"Annuler"},{L"de",L"Abbrechen"},{L"it",L"Annulla"},{L"ru",L"Отмена"},{L"zh",L"取消"},{L"ja",L"キャンセル"},{L"ko",L"취소"},{L"ar",L"إلغاء"},{L"hi",L"रद्द करें"},{L"id",L"Batal"},{L"tr",L"İptal"},{L"pl",L"Anuluj"},{L"nl",L"Annuleren"}}},
        {L"Preferências", {{L"en",L"Preferences"},{L"es",L"Preferencias"},{L"fr",L"Préférences"},{L"de",L"Einstellungen"},{L"it",L"Preferenze"},{L"ru",L"Настройки"},{L"zh",L"偏好设置"},{L"ja",L"設定"},{L"ko",L"환경설정"},{L"ar",L"التفضيلات"},{L"hi",L"प्राथमिकताएँ"},{L"id",L"Preferensi"},{L"tr",L"Tercihler"},{L"pl",L"Preferencje"},{L"nl",L"Voorkeuren"}}},
        {L"Create", {{L"en",L"Create"},{L"es",L"Crear"},{L"fr",L"Créer"},{L"de",L"Erstellen"},{L"it",L"Crea"},{L"ru",L"Создать"},{L"zh",L"创建"},{L"ja",L"作成"},{L"ko",L"만들기"},{L"ar",L"إنشاء"},{L"hi",L"बनाएँ"},{L"id",L"Buat"},{L"tr",L"Oluştur"},{L"pl",L"Utwórz"},{L"nl",L"Maken"}}},
        {L"Open", {{L"en",L"Open"},{L"es",L"Abrir"},{L"fr",L"Ouvrir"},{L"de",L"Öffnen"},{L"it",L"Apri"},{L"ru",L"Открыть"},{L"zh",L"打开"},{L"ja",L"開く"},{L"ko",L"열기"},{L"ar",L"فتح"},{L"hi",L"खोलें"},{L"id",L"Buka"},{L"tr",L"Aç"},{L"pl",L"Otwórz"},{L"nl",L"Openen"}}},
        {L"Novo projeto — configuração da tela", {{L"en",L"New project — canvas setup"},{L"es",L"Nuevo proyecto — configuración del lienzo"},{L"fr",L"Nouveau projet — configuration de la toile"},{L"de",L"Neues Projekt — Leinwand einrichten"},{L"it",L"Nuovo progetto — configurazione tela"},{L"ru",L"Новый проект — настройка холста"},{L"zh",L"新建项目 — 画布设置"},{L"ja",L"新規プロジェクト — キャンバス設定"},{L"ko",L"새 프로젝트 — 캔버스 설정"},{L"ar",L"مشروع جديد — إعداد اللوحة"},{L"hi",L"नया प्रोजेक्ट — कैनवास सेटअप"},{L"id",L"Proyek baru — pengaturan kanvas"},{L"tr",L"Yeni proje — tuval ayarları"},{L"pl",L"Nowy projekt — ustawienia płótna"},{L"nl",L"Nieuw project — canvas instellen"}}},
        {L"Largura (X):", {{L"en",L"Width (X):"},{L"es",L"Ancho (X):"},{L"fr",L"Largeur (X) :"},{L"de",L"Breite (X):"},{L"it",L"Larghezza (X):"},{L"ru",L"Ширина (X):"},{L"zh",L"宽度 (X)："},{L"ja",L"幅 (X)："},{L"ko",L"너비 (X):"},{L"ar",L"العرض (X):"},{L"hi",L"चौड़ाई (X):"},{L"id",L"Lebar (X):"},{L"tr",L"Genişlik (X):"},{L"pl",L"Szerokość (X):"},{L"nl",L"Breedte (X):"}}},
        {L"Altura (Y):", {{L"en",L"Height (Y):"},{L"es",L"Alto (Y):"},{L"fr",L"Hauteur (Y) :"},{L"de",L"Höhe (Y):"},{L"it",L"Altezza (Y):"},{L"ru",L"Высота (Y):"},{L"zh",L"高度 (Y)："},{L"ja",L"高さ (Y)："},{L"ko",L"높이 (Y):"},{L"ar",L"الارتفاع (Y):"},{L"hi",L"ऊँचाई (Y):"},{L"id",L"Tinggi (Y):"},{L"tr",L"Yükseklik (Y):"},{L"pl",L"Wysokość (Y):"},{L"nl",L"Hoogte (Y):"}}},
        {L"FPS (Taxa):", {{L"en",L"FPS (rate):"},{L"es",L"FPS (velocidad):"},{L"fr",L"IPS (vitesse) :"},{L"de",L"FPS (Rate):"},{L"it",L"FPS (frequenza):"},{L"ru",L"Кадры/с:"},{L"zh",L"帧率 FPS："},{L"ja",L"FPS（速度）："},{L"ko",L"FPS (속도):"},{L"ar",L"معدل الإطارات:"},{L"hi",L"FPS (दर):"},{L"id",L"FPS (laju):"},{L"tr",L"FPS (hız):"},{L"pl",L"FPS (klatkaż):"},{L"nl",L"FPS (snelheid):"}}},
        {L"Nome Pasta:", {{L"en",L"Folder name:"},{L"es",L"Nombre de carpeta:"},{L"fr",L"Nom du dossier :"},{L"de",L"Ordnername:"},{L"it",L"Nome cartella:"},{L"ru",L"Имя папки:"},{L"zh",L"文件夹名称："},{L"ja",L"フォルダー名："},{L"ko",L"폴더 이름:"},{L"ar",L"اسم المجلد:"},{L"hi",L"फ़ोल्डर नाम:"},{L"id",L"Nama folder:"},{L"tr",L"Klasör adı:"},{L"pl",L"Nazwa folderu:"},{L"nl",L"Mapnaam:"}}},
        {L"Diretório Raiz:", {{L"en",L"Root directory:"},{L"es",L"Directorio raíz:"},{L"fr",L"Répertoire racine :"},{L"de",L"Stammverzeichnis:"},{L"it",L"Directory principale:"},{L"ru",L"Корневая папка:"},{L"zh",L"根目录："},{L"ja",L"ルートディレクトリ："},{L"ko",L"루트 디렉터리:"},{L"ar",L"الدليل الجذر:"},{L"hi",L"मुख्य निर्देशिका:"},{L"id",L"Direktori akar:"},{L"tr",L"Kök dizin:"},{L"pl",L"Katalog główny:"},{L"nl",L"Hoofdmap:"}}},
        {L"Buscar...", {{L"en",L"Browse..."},{L"es",L"Buscar..."},{L"fr",L"Parcourir…"},{L"de",L"Durchsuchen..."},{L"it",L"Sfoglia..."},{L"ru",L"Обзор..."},{L"zh",L"浏览..."},{L"ja",L"参照..."},{L"ko",L"찾아보기..."},{L"ar",L"استعراض..."},{L"hi",L"ब्राउज़ करें..."},{L"id",L"Telusuri..."},{L"tr",L"Gözat..."},{L"pl",L"Przeglądaj..."},{L"nl",L"Bladeren..."}}},
        {L"Predefinição:", {{L"en",L"Preset:"},{L"es",L"Preajuste:"},{L"fr",L"Préréglage :"},{L"de",L"Voreinstellung:"},{L"it",L"Preimpostazione:"},{L"ru",L"Шаблон:"},{L"zh",L"预设："},{L"ja",L"プリセット："},{L"ko",L"사전 설정:"},{L"ar",L"إعداد مسبق:"},{L"hi",L"प्रीसेट:"},{L"id",L"Preset:"},{L"tr",L"Ön ayar:"},{L"pl",L"Ustawienie:"},{L"nl",L"Voorinstelling:"}}},
        {L"Personalizada", {{L"en",L"Custom"},{L"es",L"Personalizada"},{L"fr",L"Personnalisé"},{L"de",L"Benutzerdefiniert"},{L"it",L"Personalizzata"},{L"ru",L"Пользовательский"},{L"zh",L"自定义"},{L"ja",L"カスタム"},{L"ko",L"사용자 지정"},{L"ar",L"مخصص"},{L"hi",L"कस्टम"},{L"id",L"Kustom"},{L"tr",L"Özel"},{L"pl",L"Niestandardowe"},{L"nl",L"Aangepast"}}},
        {L"DPI:", {{L"en",L"DPI:"},{L"es",L"DPI:"},{L"fr",L"PPP :"},{L"de",L"DPI:"},{L"it",L"DPI:"},{L"ru",L"DPI:"},{L"zh",L"DPI："},{L"ja",L"DPI："},{L"ko",L"DPI:"},{L"ar",L"DPI:"},{L"hi",L"DPI:"},{L"id",L"DPI:"},{L"tr",L"DPI:"},{L"pl",L"DPI:"},{L"nl",L"DPI:"}}},
        {L"Pasta completa do projeto:", {{L"en",L"Full project folder:"},{L"es",L"Carpeta completa del proyecto:"},{L"fr",L"Dossier complet du projet :"},{L"de",L"Vollständiger Projektordner:"},{L"it",L"Cartella completa del progetto:"},{L"ru",L"Полная папка проекта:"},{L"zh",L"项目完整文件夹："},{L"ja",L"プロジェクトフォルダー全体："},{L"ko",L"전체 프로젝트 폴더:"},{L"ar",L"مجلد المشروع الكامل:"},{L"hi",L"पूरा प्रोजेक्ट फ़ोल्डर:"},{L"id",L"Folder proyek lengkap:"},{L"tr",L"Tam proje klasörü:"},{L"pl",L"Pełny folder projektu:"},{L"nl",L"Volledige projectmap:"}}},
        {L"PRONTO", {{L"en",L"READY"},{L"es",L"LISTO"},{L"fr",L"PRÊT"},{L"de",L"FERTIG"},{L"it",L"PRONTO"},{L"ru",L"ГОТОВО"},{L"zh",L"完成"},{L"ja",L"完了"},{L"ko",L"완료"},{L"ar",L"جاهز"},{L"hi",L"तैयार"},{L"id",L"SIAP"},{L"tr",L"HAZIR"},{L"pl",L"GOTOWE"},{L"nl",L"KLAAR"}}},
        {L"Grossura do lápis:", {{L"en",L"Pencil size:"},{L"es",L"Grosor del lápiz:"},{L"fr",L"Taille du crayon :"},{L"de",L"Stiftgröße:"},{L"it",L"Dimensione matita:"},{L"ru",L"Размер карандаша:"},{L"zh",L"铅笔粗细："},{L"ja",L"鉛筆の太さ："},{L"ko",L"연필 굵기:"},{L"ar",L"حجم القلم:"},{L"hi",L"पेंसिल आकार:"},{L"id",L"Ukuran pensil:"},{L"tr",L"Kalem boyutu:"},{L"pl",L"Rozmiar ołówka:"},{L"nl",L"Potloodgrootte:"}}},
        {L"Usar pressão da caneta", {{L"en",L"Use pen pressure"},{L"es",L"Usar presión del lápiz"},{L"fr",L"Utiliser la pression du stylet"},{L"de",L"Stiftdruck verwenden"},{L"it",L"Usa pressione penna"},{L"ru",L"Использовать нажим пера"},{L"zh",L"使用笔压"},{L"ja",L"筆圧を使用"},{L"ko",L"펜 압력 사용"},{L"ar",L"استخدام ضغط القلم"},{L"hi",L"पेन दबाव का उपयोग करें"},{L"id",L"Gunakan tekanan pena"},{L"tr",L"Kalem basıncını kullan"},{L"pl",L"Użyj nacisku pióra"},{L"nl",L"Pendruk gebruiken"}}},
        {L"Espessura mínima:", {{L"en",L"Minimum width:"},{L"es",L"Grosor mínimo:"},{L"fr",L"Épaisseur minimale :"},{L"de",L"Mindestbreite:"},{L"it",L"Spessore minimo:"},{L"ru",L"Минимальная толщина:"},{L"zh",L"最小粗细："},{L"ja",L"最小幅："},{L"ko",L"최소 굵기:"},{L"ar",L"السُمك الأدنى:"},{L"hi",L"न्यूनतम मोटाई:"},{L"id",L"Lebar minimum:"},{L"tr",L"En düşük kalınlık:"},{L"pl",L"Minimalna grubość:"},{L"nl",L"Minimale breedte:"}}},
        {L"Espessura máxima:", {{L"en",L"Maximum width:"},{L"es",L"Grosor máximo:"},{L"fr",L"Épaisseur maximale :"},{L"de",L"Maximalbreite:"},{L"it",L"Spessore massimo:"},{L"ru",L"Максимальная толщина:"},{L"zh",L"最大粗细："},{L"ja",L"最大幅："},{L"ko",L"최대 굵기:"},{L"ar",L"السُمك الأقصى:"},{L"hi",L"अधिकतम मोटाई:"},{L"id",L"Lebar maksimum:"},{L"tr",L"En yüksek kalınlık:"},{L"pl",L"Maksymalna grubość:"},{L"nl",L"Maximale breedte:"}}},
        {L"Suavizar bordas (antialiasing)", {{L"en",L"Smooth edges (anti-aliasing)"},{L"es",L"Suavizar bordes (antialiasing)"},{L"fr",L"Lisser les bords (anticrénelage)"},{L"de",L"Kanten glätten (Antialiasing)"},{L"it",L"Bordi morbidi (antialiasing)"},{L"ru",L"Сглаживание краёв"},{L"zh",L"平滑边缘（抗锯齿）"},{L"ja",L"エッジを滑らかにする"},{L"ko",L"가장자리 부드럽게"},{L"ar",L"تنعيم الحواف"},{L"hi",L"किनारे चिकने करें"},{L"id",L"Haluskan tepi (antialiasing)"},{L"tr",L"Kenarları yumuşat (kenar yumuşatma)"},{L"pl",L"Wygładzaj krawędzie"},{L"nl",L"Randen verzachten"}}},
        {L"Estabilizador do traço", {{L"en",L"Stroke stabilizer"},{L"es",L"Estabilizador del trazo"},{L"fr",L"Stabilisateur du trait"},{L"de",L"Strichstabilisierung"},{L"it",L"Stabilizzatore del tratto"},{L"ru",L"Стабилизация штриха"},{L"zh",L"笔画稳定器"},{L"ja",L"ストローク安定化"},{L"ko",L"획 안정화"},{L"ar",L"تثبيت الخط"},{L"hi",L"स्ट्रोक स्टेबलाइज़र"},{L"id",L"Stabilisator goresan"},{L"tr",L"Çizgi sabitleyici"},{L"pl",L"Stabilizator pociągnięcia"},{L"nl",L"Lijnstabilisator"}}},
        {L"Força (0-100):", {{L"en",L"Strength (0–100):"},{L"es",L"Fuerza (0–100):"},{L"fr",L"Force (0–100) :"},{L"de",L"Stärke (0–100):"},{L"it",L"Intensità (0–100):"},{L"ru",L"Сила (0–100):"},{L"zh",L"强度（0–100）："},{L"ja",L"強さ (0–100)："},{L"ko",L"강도 (0–100):"},{L"ar",L"القوة (0–100):"},{L"hi",L"तीव्रता (0–100):"},{L"id",L"Kekuatan (0–100):"},{L"tr",L"Güç (0–100):"},{L"pl",L"Siła (0–100):"},{L"nl",L"Sterkte (0–100):"}}},
        {L"Dureza do pincel:", {{L"en",L"Brush hardness:"},{L"es",L"Dureza del pincel:"},{L"fr",L"Dureté du pinceau :"},{L"de",L"Pinselhärte:"},{L"it",L"Durezza pennello:"},{L"ru",L"Жёсткость кисти:"},{L"zh",L"画笔硬度："},{L"ja",L"ブラシの硬さ："},{L"ko",L"브러시 경도:"},{L"ar",L"صلابة الفرشاة:"},{L"hi",L"ब्रश कठोरता:"},{L"id",L"Kekerasan kuas:"},{L"tr",L"Fırça sertliği:"},{L"pl",L"Twardość pędzla:"},{L"nl",L"Penseelhardheid:"}}},
        {L"Macio", {{L"en",L"Soft"},{L"es",L"Suave"},{L"fr",L"Doux"},{L"de",L"Weich"},{L"it",L"Morbido"},{L"ru",L"Мягкая"},{L"zh",L"柔软"},{L"ja",L"柔らかい"},{L"ko",L"부드러움"},{L"ar",L"ناعم"},{L"hi",L"नरम"},{L"id",L"Lembut"},{L"tr",L"Yumuşak"},{L"pl",L"Miękki"},{L"nl",L"Zacht"}}},
        {L"Suave", {{L"en",L"Smooth"},{L"es",L"Suave"},{L"fr",L"Souple"},{L"de",L"Sanft"},{L"it",L"Morbido"},{L"ru",L"Мягкая"},{L"zh",L"平滑"},{L"ja",L"滑らか"},{L"ko",L"부드러움"},{L"ar",L"ناعم"},{L"hi",L"मुलायम"},{L"id",L"Halus"},{L"tr",L"Yumuşak"},{L"pl",L"Gładki"},{L"nl",L"Glad"}}},
        {L"Firme", {{L"en",L"Firm"},{L"es",L"Firme"},{L"fr",L"Ferme"},{L"de",L"Fest"},{L"it",L"Deciso"},{L"ru",L"Плотная"},{L"zh",L"坚实"},{L"ja",L"しっかり"},{L"ko",L"단단함"},{L"ar",L"متماسك"},{L"hi",L"सख्त"},{L"id",L"Tegas"},{L"tr",L"Sert"},{L"pl",L"Mocny"},{L"nl",L"Stevig"}}},
        {L"Duro", {{L"en",L"Hard"},{L"es",L"Duro"},{L"fr",L"Dur"},{L"de",L"Hart"},{L"it",L"Duro"},{L"ru",L"Жёсткая"},{L"zh",L"硬"},{L"ja",L"硬い"},{L"ko",L"단단함"},{L"ar",L"صلب"},{L"hi",L"कठोर"},{L"id",L"Keras"},{L"tr",L"Sert"},{L"pl",L"Twardy"},{L"nl",L"Hard"}}},
        {L"Espaçamento %:", {{L"en",L"Spacing %:"},{L"es",L"Espaciado %:"},{L"fr",L"Espacement % :"},{L"de",L"Abstand %:"},{L"it",L"Spaziatura %:"},{L"ru",L"Интервал %:"},{L"zh",L"间距 %："},{L"ja",L"間隔 %："},{L"ko",L"간격 %:"},{L"ar",L"التباعد %:"},{L"hi",L"अंतर %:"},{L"id",L"Jarak %:"},{L"tr",L"Aralık %:"},{L"pl",L"Odstęp %:"},{L"nl",L"Tussenruimte %:"}}},
        {L"Ângulo °:", {{L"en",L"Angle °:"},{L"es",L"Ángulo °:"},{L"fr",L"Angle ° :"},{L"de",L"Winkel °:"},{L"it",L"Angolo °:"},{L"ru",L"Угол °:"},{L"zh",L"角度 °："},{L"ja",L"角度 °："},{L"ko",L"각도 °:"},{L"ar",L"الزاوية °:"},{L"hi",L"कोण °:"},{L"id",L"Sudut °:"},{L"tr",L"Açı °:"},{L"pl",L"Kąt °:"},{L"nl",L"Hoek °:"}}},
        {L"Redondez %:", {{L"en",L"Roundness %:"},{L"es",L"Redondez %:"},{L"fr",L"Rondeur % :"},{L"de",L"Rundheit %:"},{L"it",L"Rotondità %:"},{L"ru",L"Округлость %:"},{L"zh",L"圆度 %："},{L"ja",L"丸み %："},{L"ko",L"둥글기 %:"},{L"ar",L"الاستدارة %:"},{L"hi",L"गोलाई %:"},{L"id",L"Kebulatan %:"},{L"tr",L"Yuvarlaklık %:"},{L"pl",L"Okrągłość %:"},{L"nl",L"Rondheid %:"}}},
        {L"Dispersão %:", {{L"en",L"Scatter %:"},{L"es",L"Dispersión %:"},{L"fr",L"Dispersion % :"},{L"de",L"Streuung %:"},{L"it",L"Dispersione %:"},{L"ru",L"Рассеивание %:"},{L"zh",L"散布 %："},{L"ja",L"散布 %："},{L"ko",L"분산 %:"},{L"ar",L"التشتت %:"},{L"hi",L"फैलाव %:"},{L"id",L"Sebaran %:"},{L"tr",L"Dağılım %:"},{L"pl",L"Rozrzut %:"},{L"nl",L"Spreiding %:"}}},
        {L"Opacidade %:", {{L"en",L"Opacity %:"},{L"es",L"Opacidad %:"},{L"fr",L"Opacité % :"},{L"de",L"Deckkraft %:"},{L"it",L"Opacità %:"},{L"ru",L"Непрозрачность %:"},{L"zh",L"不透明度 %："},{L"ja",L"不透明度 %："},{L"ko",L"불투명도 %:"},{L"ar",L"العتامة %:"},{L"hi",L"अपारदर्शिता %:"},{L"id",L"Opasitas %:"},{L"tr",L"Opaklık %:"},{L"pl",L"Krycie %:"},{L"nl",L"Dekking %:"}}},
        {L"Fluxo %:", {{L"en",L"Flow %:"},{L"es",L"Flujo %:"},{L"fr",L"Flux % :"},{L"de",L"Fluss %:"},{L"it",L"Flusso %:"},{L"ru",L"Поток %:"},{L"zh",L"流量 %："},{L"ja",L"フロー %："},{L"ko",L"흐름 %:"},{L"ar",L"التدفق %:"},{L"hi",L"प्रवाह %:"},{L"id",L"Aliran %:"},{L"tr",L"Akış %:"},{L"pl",L"Przepływ %:"},{L"nl",L"Stroom %:"}}},
        {L"Girar com direção do traço", {{L"en",L"Rotate with stroke direction"},{L"es",L"Girar con la dirección del trazo"},{L"fr",L"Tourner avec le trait"},{L"de",L"Mit Strichrichtung drehen"},{L"it",L"Ruota con la direzione del tratto"},{L"ru",L"Поворот по направлению штриха"},{L"zh",L"随笔画方向旋转"},{L"ja",L"ストローク方向に回転"},{L"ko",L"획 방향으로 회전"},{L"ar",L"التدوير مع اتجاه الخط"},{L"hi",L"स्ट्रोक दिशा के साथ घुमाएँ"},{L"id",L"Putar mengikuti arah goresan"},{L"tr",L"Çizgi yönüyle döndür"},{L"pl",L"Obracaj zgodnie z kierunkiem pociągnięcia"},{L"nl",L"Draaien met lijnrichting"}}},
        {L"Tamanho por pressão", {{L"en",L"Size by pressure"},{L"es",L"Tamaño por presión"},{L"fr",L"Taille selon la pression"},{L"de",L"Größe nach Druck"},{L"it",L"Dimensione in base alla pressione"},{L"ru",L"Размер по нажиму"},{L"zh",L"按压力调整大小"},{L"ja",L"筆圧でサイズ変更"},{L"ko",L"압력에 따른 크기"},{L"ar",L"الحجم حسب الضغط"},{L"hi",L"दबाव के अनुसार आकार"},{L"id",L"Ukuran berdasarkan tekanan"},{L"tr",L"Basınca göre boyut"},{L"pl",L"Rozmiar zależny od nacisku"},{L"nl",L"Grootte op basis van druk"}}},
        {L"Opacidade por pressão", {{L"en",L"Opacity by pressure"},{L"es",L"Opacidad por presión"},{L"fr",L"Opacité selon la pression"},{L"de",L"Deckkraft nach Druck"},{L"it",L"Opacità in base alla pressione"},{L"ru",L"Непрозрачность по нажиму"},{L"zh",L"按压力调整不透明度"},{L"ja",L"筆圧で不透明度変更"},{L"ko",L"압력에 따른 불투명도"},{L"ar",L"العتامة حسب الضغط"},{L"hi",L"दबाव के अनुसार अपारदर्शिता"},{L"id",L"Opasitas berdasarkan tekanan"},{L"tr",L"Basınca göre opaklık"},{L"pl",L"Krycie zależne od nacisku"},{L"nl",L"Dekking op basis van druk"}}},
        {L"Salvar e fechar", {{L"en",L"Save and close"},{L"es",L"Guardar y cerrar"},{L"fr",L"Enregistrer et fermer"},{L"de",L"Speichern und schließen"},{L"it",L"Salva e chiudi"},{L"ru",L"Сохранить и закрыть"},{L"zh",L"保存并关闭"},{L"ja",L"保存して閉じる"},{L"ko",L"저장 후 닫기"},{L"ar",L"حفظ وإغلاق"},{L"hi",L"सहेजें और बंद करें"},{L"id",L"Simpan dan tutup"},{L"tr",L"Kaydet ve kapat"},{L"pl",L"Zapisz i zamknij"},{L"nl",L"Opslaan en sluiten"}}},
        {L"Grossura da borracha:", {{L"en",L"Eraser size:"},{L"es",L"Grosor del borrador:"},{L"fr",L"Taille de la gomme :"},{L"de",L"Radierergröße:"},{L"it",L"Dimensione gomma:"},{L"ru",L"Размер ластика:"},{L"zh",L"橡皮擦大小："},{L"ja",L"消しゴムのサイズ："},{L"ko",L"지우개 크기:"},{L"ar",L"حجم الممحاة:"},{L"hi",L"इरेज़र आकार:"},{L"id",L"Ukuran penghapus:"},{L"tr",L"Silgi boyutu:"},{L"pl",L"Rozmiar gumki:"},{L"nl",L"Grootte gum:"}}},
        {L"Usar pressão na borracha", {{L"en",L"Use pressure on eraser"},{L"es",L"Usar presión en el borrador"},{L"fr",L"Utiliser la pression sur la gomme"},{L"de",L"Druck für Radierer verwenden"},{L"it",L"Usa pressione sulla gomma"},{L"ru",L"Использовать нажим для ластика"},{L"zh",L"橡皮擦使用笔压"},{L"ja",L"消しゴムに筆圧を使用"},{L"ko",L"지우개 압력 사용"},{L"ar",L"استخدام الضغط للممحاة"},{L"hi",L"इरेज़र पर दबाव उपयोग करें"},{L"id",L"Gunakan tekanan pada penghapus"},{L"tr",L"Silgide basınç kullan"},{L"pl",L"Użyj nacisku w gumce"},{L"nl",L"Druk gebruiken voor gum"}}},
        {L"Propriedades do lápis", {{L"en",L"Pencil properties"},{L"es",L"Propiedades del lápiz"},{L"fr",L"Propriétés du crayon"},{L"de",L"Stifteigenschaften"},{L"it",L"Proprietà matita"},{L"ru",L"Свойства карандаша"},{L"zh",L"铅笔属性"},{L"ja",L"鉛筆のプロパティ"},{L"ko",L"연필 속성"},{L"ar",L"خصائص القلم"},{L"hi",L"पेंसिल गुण"},{L"id",L"Properti pensil"},{L"tr",L"Kalem özellikleri"},{L"pl",L"Właściwości ołówka"},{L"nl",L"Penseeleigenschappen"}}},
        {L"Propriedades da borracha", {{L"en",L"Eraser properties"},{L"es",L"Propiedades del borrador"},{L"fr",L"Propriétés de la gomme"},{L"de",L"Radierereigenschaften"},{L"it",L"Proprietà gomma"},{L"ru",L"Свойства ластика"},{L"zh",L"橡皮擦属性"},{L"ja",L"消しゴムのプロパティ"},{L"ko",L"지우개 속성"},{L"ar",L"خصائص الممحاة"},{L"hi",L"इरेज़र गुण"},{L"id",L"Properti penghapus"},{L"tr",L"Silgi özellikleri"},{L"pl",L"Właściwości gumki"},{L"nl",L"Gumeigenschappen"}}},
        {L"Propriedades do balde", {{L"en",L"Bucket properties"},{L"es",L"Propiedades del cubo"},{L"fr",L"Propriétés du pot"},{L"de",L"Fülleimer-Eigenschaften"},{L"it",L"Proprietà secchiello"},{L"ru",L"Свойства заливки"},{L"zh",L"油漆桶属性"},{L"ja",L"塗りつぶしのプロパティ"},{L"ko",L"채우기 속성"},{L"ar",L"خصائص الدلو"},{L"hi",L"बकेट गुण"},{L"id",L"Properti ember"},{L"tr",L"Boya kovası özellikleri"},{L"pl",L"Właściwości wiadra"},{L"nl",L"Emmereigenschappen"}}},
        {L"Propriedades do laço", {{L"en",L"Lasso properties"},{L"es",L"Propiedades del lazo"},{L"fr",L"Propriétés du lasso"},{L"de",L"Lasso-Eigenschaften"},{L"it",L"Proprietà lazo"},{L"ru",L"Свойства лассо"},{L"zh",L"套索属性"},{L"ja",L"投げ縄のプロパティ"},{L"ko",L"올가미 속성"},{L"ar",L"خصائص التحديد الحر"},{L"hi",L"लैसो गुण"},{L"id",L"Properti laso"},{L"tr",L"Kement özellikleri"},{L"pl",L"Właściwości lassa"},{L"nl",L"Lasso-eigenschappen"}}},
        {L"Modo de seleção:", {{L"en",L"Selection mode:"},{L"es",L"Modo de selección:"},{L"fr",L"Mode de sélection :"},{L"de",L"Auswahlmodus:"},{L"it",L"Modalità selezione:"},{L"ru",L"Режим выделения:"},{L"zh",L"选择模式："},{L"ja",L"選択モード："},{L"ko",L"선택 모드:"},{L"ar",L"وضع التحديد:"},{L"hi",L"चयन मोड:"},{L"id",L"Mode seleksi:"},{L"tr",L"Seçim modu:"},{L"pl",L"Tryb zaznaczania:"},{L"nl",L"Selectiemodus:"}}},
        {L"Retangular", {{L"en",L"Rectangular"},{L"es",L"Rectangular"},{L"fr",L"Rectangulaire"},{L"de",L"Rechteckig"},{L"it",L"Rettangolare"},{L"ru",L"Прямоугольное"},{L"zh",L"矩形"},{L"ja",L"長方形"},{L"ko",L"직사각형"},{L"ar",L"مستطيل"},{L"hi",L"आयताकार"},{L"id",L"Persegi panjang"},{L"tr",L"Dikdörtgen"},{L"pl",L"Prostokątne"},{L"nl",L"Rechthoekig"}}},
        {L"Desenhar área livre", {{L"en",L"Freehand area"},{L"es",L"Dibujar área libre"},{L"fr",L"Zone libre"},{L"de",L"Freihandbereich"},{L"it",L"Area a mano libera"},{L"ru",L"Свободная область"},{L"zh",L"自由绘制区域"},{L"ja",L"フリーハンド領域"},{L"ko",L"자유 영역"},{L"ar",L"منطقة حرة"},{L"hi",L"मुक्त क्षेत्र"},{L"id",L"Area bebas"},{L"tr",L"Serbest alan"},{L"pl",L"Obszar odręczny"},{L"nl",L"Vrije hand"}}},
        {L"Escolher cor da seleção...", {{L"en",L"Choose selection color..."},{L"es",L"Elegir color de selección..."},{L"fr",L"Choisir la couleur de sélection…"},{L"de",L"Auswahlfarbe wählen..."},{L"it",L"Scegli colore selezione..."},{L"ru",L"Выбрать цвет выделения..."},{L"zh",L"选择选区颜色..."},{L"ja",L"選択色を選ぶ..."},{L"ko",L"선택 색상 선택..."},{L"ar",L"اختر لون التحديد..."},{L"hi",L"चयन रंग चुनें..."},{L"id",L"Pilih warna seleksi..."},{L"tr",L"Seçim rengini seç..."},{L"pl",L"Wybierz kolor zaznaczenia..."},{L"nl",L"Selectiekleur kiezen..."}}},
        {L"Salvar", {{L"en",L"Save"},{L"es",L"Guardar"},{L"fr",L"Enregistrer"},{L"de",L"Speichern"},{L"it",L"Salva"},{L"ru",L"Сохранить"},{L"zh",L"保存"},{L"ja",L"保存"},{L"ko",L"저장"},{L"ar",L"حفظ"},{L"hi",L"सहेजें"},{L"id",L"Simpan"},{L"tr",L"Kaydet"},{L"pl",L"Zapisz"},{L"nl",L"Opslaan"}}},
        {L"Fechar", {{L"en",L"Close"},{L"es",L"Cerrar"},{L"fr",L"Fermer"},{L"de",L"Schließen"},{L"it",L"Chiudi"},{L"ru",L"Закрыть"},{L"zh",L"关闭"},{L"ja",L"閉じる"},{L"ko",L"닫기"},{L"ar",L"إغلاق"},{L"hi",L"बंद करें"},{L"id",L"Tutup"},{L"tr",L"Kapat"},{L"pl",L"Zamknij"},{L"nl",L"Sluiten"}}},
        {L"Renderizar", {{L"en",L"Render"},{L"es",L"Renderizar"},{L"fr",L"Rendu"},{L"de",L"Rendern"},{L"it",L"Renderizza"},{L"ru",L"Рендер"},{L"zh",L"渲染"},{L"ja",L"レンダリング"},{L"ko",L"렌더링"},{L"ar",L"تصيير"},{L"hi",L"रेंडर"},{L"id",L"Render"},{L"tr",L"Oluştur"},{L"pl",L"Renderuj"},{L"nl",L"Renderen"}}}
    };
    return t;
}

// Catálogo suplementar das janelas que eram criadas com legendas fora do
// primeiro conjunto: biblioteca/propriedades de pincéis, MKV, ferramentas,
// timeline, preferências e renderização. As chaves de origem ficam estáveis
// para que trocar de idioma repetidamente não degrade as legendas.
static const std::map<std::wstring,std::map<std::wstring,std::wstring> >& KodaCatalogoExtraTraducao() {
    static const std::map<std::wstring,std::map<std::wstring,std::wstring> > t = {
        {L"Pincéis do Kodanima", {{L"en",L"Kodanima Brushes"},{L"es",L"Pinceles de Kodanima"},{L"fr",L"Pinceaux Kodanima"},{L"de",L"Kodanima-Pinsel"},{L"it",L"Pennelli Kodanima"},{L"ru",L"Кисти Kodanima"},{L"zh",L"Kodanima 笔刷"},{L"ja",L"Kodanima ブラシ"},{L"ko",L"Kodanima 브러시"},{L"nl",L"Kodanima-penselen"},{L"tr",L"Kodanima Fırçaları"},{L"pl",L"Pędzle Kodanima"},{L"id",L"Kuas Kodanima"},{L"ar",L"فرش Kodanima"},{L"hi",L"Kodanima ब्रश"}}},
        {L"Gerenciador de pincéis", {{L"en",L"Brush Manager"},{L"es",L"Administrador de pinceles"},{L"fr",L"Gestionnaire de pinceaux"},{L"de",L"Pinselverwaltung"},{L"it",L"Gestione pennelli"},{L"ru",L"Менеджер кистей"},{L"zh",L"笔刷管理器"},{L"ja",L"ブラシ管理"},{L"ko",L"브러시 관리자"},{L"nl",L"Penseelbeheer"},{L"tr",L"Fırça Yöneticisi"},{L"pl",L"Menedżer pędzli"},{L"id",L"Pengelola kuas"},{L"ar",L"مدير الفرش"},{L"hi",L"ब्रश प्रबंधक"}}},
        {L"Pincéis importados", {{L"en",L"Imported brushes"},{L"es",L"Pinceles importados"},{L"fr",L"Pinceaux importés"},{L"de",L"Importierte Pinsel"},{L"it",L"Pennelli importati"},{L"ru",L"Импортированные кисти"},{L"zh",L"已导入笔刷"},{L"ja",L"インポートしたブラシ"},{L"ko",L"가져온 브러시"},{L"nl",L"Geïmporteerde penselen"},{L"tr",L"İçe aktarılan fırçalar"},{L"pl",L"Importowane pędzle"},{L"id",L"Kuas yang diimpor"},{L"ar",L"الفرش المستوردة"},{L"hi",L"आयात किए गए ब्रश"}}},
        {L"Prévia do pincel", {{L"en",L"Brush preview"},{L"es",L"Vista previa del pincel"},{L"fr",L"Aperçu du pinceau"},{L"de",L"Pinselvorschau"},{L"it",L"Anteprima pennello"},{L"ru",L"Предпросмотр кисти"},{L"zh",L"笔刷预览"},{L"ja",L"ブラシプレビュー"},{L"ko",L"브러시 미리 보기"},{L"nl",L"Penseelvoorbeeld"},{L"tr",L"Fırça önizlemesi"},{L"pl",L"Podgląd pędzla"},{L"id",L"Pratinjau kuas"},{L"ar",L"معاينة الفرشاة"},{L"hi",L"ब्रश पूर्वावलोकन"}}},
        {L"X Excluir", {{L"en",L"X Delete"},{L"es",L"X Eliminar"},{L"fr",L"X Supprimer"},{L"de",L"X Löschen"},{L"it",L"X Elimina"},{L"ru",L"X Удалить"},{L"zh",L"X 删除"},{L"ja",L"X 削除"},{L"ko",L"X 삭제"},{L"nl",L"X Verwijderen"},{L"tr",L"X Sil"},{L"pl",L"X Usuń"},{L"id",L"X Hapus"},{L"ar",L"X حذف"},{L"hi",L"X हटाएं"}}},
        {L"⚙ Propriedades", {{L"en",L"⚙ Properties"},{L"es",L"⚙ Propiedades"},{L"fr",L"⚙ Propriétés"},{L"de",L"⚙ Eigenschaften"},{L"it",L"⚙ Proprietà"},{L"ru",L"⚙ Свойства"},{L"zh",L"⚙ 属性"},{L"ja",L"⚙ プロパティ"},{L"ko",L"⚙ 속성"},{L"nl",L"⚙ Eigenschappen"},{L"tr",L"⚙ Özellikler"},{L"pl",L"⚙ Właściwości"},{L"id",L"⚙ Properti"},{L"ar",L"⚙ الخصائص"},{L"hi",L"⚙ गुण"}}},
        {L"Textura...", {{L"en",L"Texture..."},{L"es",L"Textura..."},{L"fr",L"Texture..."},{L"de",L"Textur..."},{L"it",L"Texture..."},{L"ru",L"Текстура..."},{L"zh",L"纹理..."},{L"ja",L"テクスチャ..."},{L"ko",L"텍스처..."},{L"nl",L"Textuur..."},{L"tr",L"Doku..."},{L"pl",L"Tekstura..."},{L"id",L"Tekstur..."},{L"ar",L"ملمس..."},{L"hi",L"बनावट..."}}},
        {L"Renomear", {{L"en",L"Rename"},{L"es",L"Renombrar"},{L"fr",L"Renommer"},{L"de",L"Umbenennen"},{L"it",L"Rinomina"},{L"ru",L"Переименовать"},{L"zh",L"重命名"},{L"ja",L"名前を変更"},{L"ko",L"이름 변경"},{L"nl",L"Hernoemen"},{L"tr",L"Yeniden adlandır"},{L"pl",L"Zmień nazwę"},{L"id",L"Ganti nama"},{L"ar",L"إعادة تسمية"},{L"hi",L"नाम बदलें"}}},
        {L"Mecânicas (MKV)", {{L"en",L"Mechanics (MKV)"},{L"es",L"Mecánicas (MKV)"},{L"fr",L"Mécaniques (MKV)"},{L"de",L"Mechaniken (MKV)"},{L"it",L"Meccaniche (MKV)"},{L"ru",L"Механики (MKV)"},{L"zh",L"机制 (MKV)"},{L"ja",L"メカニクス (MKV)"},{L"ko",L"메커니즘 (MKV)"},{L"nl",L"Mechanica (MKV)"},{L"tr",L"Mekanikler (MKV)"},{L"pl",L"Mechanika (MKV)"},{L"id",L"Mekanik (MKV)"},{L"ar",L"آليات (MKV)"},{L"hi",L"मैकेनिक्स (MKV)"}}},
        {L"Propriedades do pincel", {{L"en",L"Brush Properties"},{L"es",L"Propiedades del pincel"},{L"fr",L"Propriétés du pinceau"},{L"de",L"Pinseleigenschaften"},{L"it",L"Proprietà pennello"},{L"ru",L"Свойства кисти"},{L"zh",L"笔刷属性"},{L"ja",L"ブラシのプロパティ"},{L"ko",L"브러시 속성"},{L"nl",L"Penseeleigenschappen"},{L"tr",L"Fırça özellikleri"},{L"pl",L"Właściwości pędzla"},{L"id",L"Properti kuas"},{L"ar",L"خصائص الفرشاة"},{L"hi",L"ब्रश गुण"}}},
        {L"Propriedades do lápis", {{L"en",L"Pencil Properties"},{L"es",L"Propiedades del lápiz"},{L"fr",L"Propriétés du crayon"},{L"de",L"Stifteigenschaften"},{L"it",L"Proprietà matita"},{L"ru",L"Свойства карандаша"},{L"zh",L"铅笔属性"},{L"ja",L"鉛筆のプロパティ"},{L"ko",L"연필 속성"},{L"nl",L"Potloodeigenschappen"},{L"tr",L"Kalem özellikleri"},{L"pl",L"Właściwości ołówka"},{L"id",L"Properti pensil"},{L"ar",L"خصائص القلم"},{L"hi",L"पेंसिल गुण"}}},
        {L"Propriedades da borracha", {{L"en",L"Eraser Properties"},{L"es",L"Propiedades del borrador"},{L"fr",L"Propriétés de la gomme"},{L"de",L"Radierereigenschaften"},{L"it",L"Proprietà gomma"},{L"ru",L"Свойства ластика"},{L"zh",L"橡皮属性"},{L"ja",L"消しゴムのプロパティ"},{L"ko",L"지우개 속성"},{L"nl",L"Gum-eigenschappen"},{L"tr",L"Silgi özellikleri"},{L"pl",L"Właściwości gumki"},{L"id",L"Properti penghapus"},{L"ar",L"خصائص الممحاة"},{L"hi",L"इरेज़र गुण"}}},
        {L"Propriedades do balde", {{L"en",L"Bucket Properties"},{L"es",L"Propiedades del bote"},{L"fr",L"Propriétés du pot"},{L"de",L"Fülleimer-Eigenschaften"},{L"it",L"Proprietà secchiello"},{L"ru",L"Свойства заливки"},{L"zh",L"油漆桶属性"},{L"ja",L"バケツのプロパティ"},{L"ko",L"채우기 속성"},{L"nl",L"Emmereigenschappen"},{L"tr",L"Boya kovası özellikleri"},{L"pl",L"Właściwości wiadra"},{L"id",L"Properti ember"},{L"ar",L"خصائص التعبئة"},{L"hi",L"बकेट गुण"}}},
        {L"Propriedades do laço", {{L"en",L"Lasso Properties"},{L"es",L"Propiedades del lazo"},{L"fr",L"Propriétés du lasso"},{L"de",L"Lasso-Eigenschaften"},{L"it",L"Proprietà lazo"},{L"ru",L"Свойства лассо"},{L"zh",L"套索属性"},{L"ja",L"投げ縄のプロパティ"},{L"ko",L"올가미 속성"},{L"nl",L"Lasso-eigenschappen"},{L"tr",L"Kement özellikleri"},{L"pl",L"Właściwości lassa"},{L"id",L"Properti laso"},{L"ar",L"خصائص اللاسو"},{L"hi",L"लासो गुण"}}},
        {L"Pincéis importados: ATIVADOS (desmarque para usar o pincel padrão da engine)", {{L"en",L"Imported brushes: ON (uncheck to use the engine's default brush)"},{L"es",L"Pinceles importados: ACTIVADOS (desmarque para usar el pincel predeterminado)"},{L"fr",L"Pinceaux importés : ACTIVÉS (décocher pour utiliser le pinceau par défaut)"},{L"de",L"Importierte Pinsel: EIN (deaktivieren für Standardpinsel)"},{L"it",L"Pennelli importati: ATTIVI (deseleziona per usare il pennello predefinito)"},{L"ru",L"Импортированные кисти: ВКЛ. (снимите флажок для стандартной кисти)"},{L"zh",L"导入笔刷：开启（取消勾选以使用默认笔刷）"},{L"ja",L"インポートブラシ：オン（既定のブラシを使うには解除）"},{L"ko",L"가져온 브러시: 켜짐(기본 브러시를 사용하려면 해제)"},{L"nl",L"Geïmporteerde penselen: AAN (uitvinken voor standaardpenseel)"},{L"tr",L"İçe aktarılan fırçalar: AÇIK (varsayılan fırça için işareti kaldırın)"},{L"pl",L"Importowane pędzle: WŁ. (odznacz, aby użyć domyślnego pędzla)"},{L"id",L"Kuas impor: AKTIF (hapus centang untuk kuas bawaan)"},{L"ar",L"الفرش المستوردة: تشغيل (ألغِ التحديد لاستخدام الفرشاة الافتراضية)"},{L"hi",L"आयातित ब्रश: चालू (डिफ़ॉल्ट ब्रश के लिए अनचेक करें)"}}},
        {L"Antialiasing da ponta", {{L"en",L"Tip antialiasing"},{L"es",L"Antialiasing de la punta"},{L"fr",L"Anticrénelage de la pointe"},{L"de",L"Kantenglättung der Spitze"},{L"it",L"Antialiasing punta"},{L"ru",L"Сглаживание кончика"},{L"zh",L"笔尖抗锯齿"},{L"ja",L"先端のアンチエイリアス"},{L"ko",L"브러시 끝 안티앨리어싱"},{L"nl",L"Punt-antialiasing"},{L"tr",L"Uç kenar yumuşatma"},{L"pl",L"Wygładzanie końcówki"},{L"id",L"Antialias ujung"},{L"ar",L"تنعيم حافة الطرف"},{L"hi",L"टिप एंटीएलियासिंग"}}},
        {L"MKV reúne rotação, escala aleatória, partículas, dispersão e blur. As categorias à esquerda seguem a organização inspirada em Sub Tool Detail.", {{L"en",L"MKV combines rotation, random scale, particles, scatter and blur. Categories follow a Sub Tool Detail-inspired layout."},{L"es",L"MKV combina rotación, escala aleatoria, partículas, dispersión y desenfoque. Las categorías siguen una organización inspirada en Sub Tool Detail."},{L"fr",L"MKV combine rotation, échelle aléatoire, particules, dispersion et flou. Les catégories s'inspirent de Sub Tool Detail."},{L"de",L"MKV kombiniert Drehung, Zufallsskalierung, Partikel, Streuung und Weichzeichnung. Die Kategorien orientieren sich an Sub Tool Detail."},{L"it",L"MKV combina rotazione, scala casuale, particelle, dispersione e sfocatura. Le categorie si ispirano a Sub Tool Detail."},{L"ru",L"MKV объединяет вращение, случайный масштаб, частицы, разброс и размытие. Категории основаны на Sub Tool Detail."},{L"zh",L"MKV 集成旋转、随机缩放、粒子、散布和模糊。分类布局参考 Sub Tool Detail。"},{L"ja",L"MKV は回転、ランダム拡大縮小、パーティクル、散布、ぼかしを統合します。分類は Sub Tool Detail を参考にしています。"},{L"ko",L"MKV는 회전, 임의 크기, 입자, 분산 및 흐림을 통합합니다. 범주는 Sub Tool Detail 형식을 참고합니다。"}}},
        {L"Acesso rápido a todas as configurações Sub Tool Detail:", {{L"en",L"Quick access to all Sub Tool Detail settings:"},{L"es",L"Acceso rápido a todos los ajustes de Sub Tool Detail:"},{L"fr",L"Accès rapide à tous les réglages Sub Tool Detail :"},{L"de",L"Schnellzugriff auf alle Sub-Tool-Detail-Einstellungen:"},{L"it",L"Accesso rapido a tutte le impostazioni Sub Tool Detail:"},{L"ru",L"Быстрый доступ ко всем настройкам Sub Tool Detail:"},{L"zh",L"快速访问所有 Sub Tool Detail 设置："},{L"ja",L"すべての Sub Tool Detail 設定へのクイックアクセス："},{L"ko",L"모든 Sub Tool Detail 설정에 빠르게 액세스:"}}},
        {L"Brush Tip", {{L"en",L"Brush Tip"},{L"es",L"Punta del pincel"},{L"fr",L"Pointe du pinceau"},{L"de",L"Pinselspitze"},{L"it",L"Punta pennello"},{L"ru",L"Кончик кисти"},{L"zh",L"笔刷笔尖"},{L"ja",L"ブラシ先端"},{L"ko",L"브러시 끝"},{L"nl",L"Penseelpunt"},{L"tr",L"Fırça ucu"},{L"pl",L"Końcówka pędzla"},{L"id",L"Ujung kuas"}}},
        {L"Stroke", {{L"en",L"Stroke"},{L"es",L"Trazo"},{L"fr",L"Trait"},{L"de",L"Strich"},{L"it",L"Tratto"},{L"ru",L"Штрих"},{L"zh",L"笔画"},{L"ja",L"ストローク"},{L"ko",L"획"},{L"nl",L"Lijn"},{L"tr",L"Çizgi"},{L"pl",L"Pociągnięcie"},{L"id",L"Goresan"}}},
        {L"Texture", {{L"en",L"Texture"},{L"es",L"Textura"},{L"fr",L"Texture"},{L"de",L"Textur"},{L"it",L"Trama"},{L"ru",L"Текстура"},{L"zh",L"纹理"},{L"ja",L"テクスチャ"},{L"ko",L"텍스처"},{L"nl",L"Textuur"},{L"tr",L"Doku"},{L"pl",L"Tekstura"},{L"id",L"Tekstur"}}},
        {L"Spray", {{L"en",L"Spray"},{L"es",L"Aerosol"},{L"fr",L"Pulvérisation"},{L"de",L"Spray"},{L"it",L"Spruzzo"},{L"ru",L"Распыление"},{L"zh",L"喷涂"},{L"ja",L"スプレー"},{L"ko",L"스프레이"},{L"nl",L"Spray"},{L"tr",L"Sprey"},{L"pl",L"Spray"},{L"id",L"Semprot"}}},
        {L"Start/End", {{L"en",L"Start/End"},{L"es",L"Inicio/Fin"},{L"fr",L"Début/Fin"},{L"de",L"Anfang/Ende"},{L"it",L"Inizio/Fine"},{L"ru",L"Начало/Конец"},{L"zh",L"起始/结束"},{L"ja",L"開始/終了"},{L"ko",L"시작/끝"}}},
        {L"Ink/Pressão", {{L"en",L"Ink/Pressure"},{L"es",L"Tinta/Presión"},{L"fr",L"Encre/Pression"},{L"de",L"Tinte/Druck"},{L"it",L"Inchiostro/Pressione"},{L"ru",L"Чернила/Нажим"},{L"zh",L"墨水/压力"},{L"ja",L"インク/筆圧"},{L"ko",L"잉크/압력"}}},
        {L"Anti-overflow", {{L"en",L"Anti-overflow"},{L"es",L"Antidesbordamiento"},{L"fr",L"Anti-débordement"},{L"de",L"Überlauf verhindern"},{L"it",L"Anti-fuoriuscita"},{L"ru",L"Защита от выхода"},{L"zh",L"防溢出"},{L"ja",L"はみ出し防止"},{L"ko",L"넘침 방지"}}},
        {L"Salvar", {{L"en",L"Save"},{L"es",L"Guardar"},{L"fr",L"Enregistrer"},{L"de",L"Speichern"},{L"it",L"Salva"},{L"ru",L"Сохранить"},{L"zh",L"保存"},{L"ja",L"保存"},{L"ko",L"저장"},{L"nl",L"Opslaan"},{L"tr",L"Kaydet"},{L"pl",L"Zapisz"},{L"id",L"Simpan"},{L"ar",L"حفظ"},{L"hi",L"सहेजें"}}},
        {L"Voltar às categorias", {{L"en",L"Back to categories"},{L"es",L"Volver a las categorías"},{L"fr",L"Retour aux catégories"},{L"de",L"Zurück zu den Kategorien"},{L"it",L"Torna alle categorie"},{L"ru",L"Назад к категориям"},{L"zh",L"返回类别"},{L"ja",L"カテゴリに戻る"},{L"ko",L"범주로 돌아가기"},{L"nl",L"Terug naar categorieën"},{L"tr",L"Kategorilere dön"},{L"pl",L"Wróć do kategorii"},{L"id",L"Kembali ke kategori"}}},
        {L"Cancelar", {{L"en",L"Cancel"},{L"es",L"Cancelar"},{L"fr",L"Annuler"},{L"de",L"Abbrechen"},{L"it",L"Annulla"},{L"ru",L"Отмена"},{L"zh",L"取消"},{L"ja",L"キャンセル"},{L"ko",L"취소"},{L"nl",L"Annuleren"},{L"tr",L"İptal"},{L"pl",L"Anuluj"},{L"id",L"Batal"},{L"ar",L"إلغاء"},{L"hi",L"रद्द करें"}}},
        {L"CAMADAS / ORDEM", {{L"en",L"LAYERS / ORDER"},{L"es",L"CAPAS / ORDEN"},{L"fr",L"CALQUES / ORDRE"},{L"de",L"EBENEN / REIHENFOLGE"},{L"it",L"LIVELLI / ORDINE"},{L"ru",L"СЛОИ / ПОРЯДОК"},{L"zh",L"图层 / 顺序"},{L"ja",L"レイヤー / 順序"},{L"ko",L"레이어 / 순서"},{L"nl",L"LAGEN / VOLGORDE"},{L"tr",L"KATMANLAR / SIRA"},{L"pl",L"WARSTWY / KOLEJNOŚĆ"},{L"id",L"LAPISAN / URUTAN"}}},
        {L"Timeline: tema", {{L"en",L"Timeline: theme"},{L"es",L"Línea de tiempo: tema"},{L"fr",L"Chronologie : thème"},{L"de",L"Zeitleiste: Thema"},{L"it",L"Timeline: tema"},{L"ru",L"Таймлайн: тема"},{L"zh",L"时间轴：主题"},{L"ja",L"タイムライン：テーマ"},{L"ko",L"타임라인: 테마"}}},
        {L"Timeline: cor", {{L"en",L"Timeline: color"},{L"es",L"Línea de tiempo: color"},{L"fr",L"Chronologie : couleur"},{L"de",L"Zeitleiste: Farbe"},{L"it",L"Timeline: colore"},{L"ru",L"Таймлайн: цвет"},{L"zh",L"时间轴：颜色"},{L"ja",L"タイムライン：色"},{L"ko",L"타임라인: 색상"}}},
        {L"Play", {{L"en",L"Play"},{L"es",L"Reproducir"},{L"fr",L"Lire"},{L"de",L"Abspielen"},{L"it",L"Riproduci"},{L"ru",L"Воспроизвести"},{L"zh",L"播放"},{L"ja",L"再生"},{L"ko",L"재생"},{L"nl",L"Afspelen"},{L"tr",L"Oynat"},{L"pl",L"Odtwórz"},{L"id",L"Putar"}}},
        {L"Loop", {{L"en",L"Loop"},{L"es",L"Bucle"},{L"fr",L"Boucle"},{L"de",L"Schleife"},{L"it",L"Ciclo"},{L"ru",L"Цикл"},{L"zh",L"循环"},{L"ja",L"ループ"},{L"ko",L"반복"},{L"nl",L"Herhalen"},{L"tr",L"Döngü"},{L"pl",L"Pętla"},{L"id",L"Ulang"}}},
        {L"+ Camada", {{L"en",L"+ Layer"},{L"es",L"+ Capa"},{L"fr",L"+ Calque"},{L"de",L"+ Ebene"},{L"it",L"+ Livello"},{L"ru",L"+ Слой"},{L"zh",L"+ 图层"},{L"ja",L"+ レイヤー"},{L"ko",L"+ 레이어"},{L"nl",L"+ Laag"},{L"tr",L"+ Katman"},{L"pl",L"+ Warstwa"},{L"id",L"+ Lapisan"}}},
        {L"Remover", {{L"en",L"Remove"},{L"es",L"Quitar"},{L"fr",L"Retirer"},{L"de",L"Entfernen"},{L"it",L"Rimuovi"},{L"ru",L"Удалить"},{L"zh",L"移除"},{L"ja",L"削除"},{L"ko",L"제거"},{L"nl",L"Verwijderen"},{L"tr",L"Kaldır"},{L"pl",L"Usuń"},{L"id",L"Hapus"}}},
        {L"Onion", {{L"en",L"Onion skin"},{L"es",L"Piel de cebolla"},{L"fr",L"Pelure d'oignon"},{L"de",L"Zwiebelschale"},{L"it",L"Pelle di cipolla"},{L"ru",L"Калька"},{L"zh",L"洋葱皮"},{L"ja",L"オニオンスキン"},{L"ko",L"어니언 스킨"}}},
        {L"+ Quadro", {{L"en",L"+ Frame"},{L"es",L"+ Fotograma"},{L"fr",L"+ Image"},{L"de",L"+ Einzelbild"},{L"it",L"+ Fotogramma"},{L"ru",L"+ Кадр"},{L"zh",L"+ 帧"},{L"ja",L"+ フレーム"},{L"ko",L"+ 프레임"}}},
        {L"Inserir", {{L"en",L"Insert"},{L"es",L"Insertar"},{L"fr",L"Insérer"},{L"de",L"Einfügen"},{L"it",L"Inserisci"},{L"ru",L"Вставить"},{L"zh",L"插入"},{L"ja",L"挿入"},{L"ko",L"삽입"}}},
        {L"Excluir", {{L"en",L"Delete"},{L"es",L"Eliminar"},{L"fr",L"Supprimer"},{L"de",L"Löschen"},{L"it",L"Elimina"},{L"ru",L"Удалить"},{L"zh",L"删除"},{L"ja",L"削除"},{L"ko",L"삭제"}}},
        {L"Dur.", {{L"en",L"Dur."},{L"es",L"Dur."},{L"fr",L"Durée"},{L"de",L"Dauer"},{L"it",L"Dur."},{L"ru",L"Длит."},{L"zh",L"时长"},{L"ja",L"長さ"},{L"ko",L"길이"}}},
        {L"Idioma", {{L"en",L"Language"},{L"es",L"Idioma"},{L"fr",L"Langue"},{L"de",L"Sprache"},{L"it",L"Lingua"},{L"ru",L"Язык"},{L"zh",L"语言"},{L"ja",L"言語"},{L"ko",L"언어"}}},
        {L"Kodanima — Renderizar animação", {{L"en",L"Kodanima — Render animation"},{L"es",L"Kodanima — Renderizar animación"},{L"fr",L"Kodanima — Rendu de l'animation"},{L"de",L"Kodanima — Animation rendern"},{L"it",L"Kodanima — Render animazione"},{L"ru",L"Kodanima — Рендер анимации"},{L"zh",L"Kodanima — 渲染动画"},{L"ja",L"Kodanima — アニメーションを書き出す"},{L"ko",L"Kodanima — 애니메이션 렌더링"}}},
        {L"Formato de arquivo:", {{L"en",L"File format:"},{L"es",L"Formato de archivo:"},{L"fr",L"Format de fichier :"},{L"de",L"Dateiformat:"},{L"it",L"Formato file:"},{L"ru",L"Формат файла:"},{L"zh",L"文件格式："},{L"ja",L"ファイル形式："},{L"ko",L"파일 형식:"}}},
        {L"Nome do arquivo:", {{L"en",L"File name:"},{L"es",L"Nombre del archivo:"},{L"fr",L"Nom du fichier :"},{L"de",L"Dateiname:"},{L"it",L"Nome file:"},{L"ru",L"Имя файла:"},{L"zh",L"文件名："},{L"ja",L"ファイル名："},{L"ko",L"파일 이름:"}}},
        {L"Diretório de saída:", {{L"en",L"Output folder:"},{L"es",L"Carpeta de salida:"},{L"fr",L"Dossier de sortie :"},{L"de",L"Ausgabeordner:"},{L"it",L"Cartella di output:"},{L"ru",L"Папка вывода:"},{L"zh",L"输出文件夹："},{L"ja",L"出力フォルダー："},{L"ko",L"출력 폴더:"}}},
        {L"Procurar...", {{L"en",L"Browse..."},{L"es",L"Examinar..."},{L"fr",L"Parcourir..."},{L"de",L"Durchsuchen..."},{L"it",L"Sfoglia..."},{L"ru",L"Обзор..."},{L"zh",L"浏览..."},{L"ja",L"参照..."},{L"ko",L"찾아보기..."}}},
        {L"Fundo transparente (GIF/MOV)", {{L"en",L"Transparent background (GIF/MOV)"},{L"es",L"Fondo transparente (GIF/MOV)"},{L"fr",L"Arrière-plan transparent (GIF/MOV)"},{L"de",L"Transparenter Hintergrund (GIF/MOV)"},{L"it",L"Sfondo trasparente (GIF/MOV)"},{L"ru",L"Прозрачный фон (GIF/MOV)"},{L"zh",L"透明背景 (GIF/MOV)"},{L"ja",L"透明な背景 (GIF/MOV)"},{L"ko",L"투명 배경 (GIF/MOV)"}}},
        {L"Renderizar", {{L"en",L"Render"},{L"es",L"Renderizar"},{L"fr",L"Rendre"},{L"de",L"Rendern"},{L"it",L"Renderizza"},{L"ru",L"Рендер"},{L"zh",L"渲染"},{L"ja",L"レンダリング"},{L"ko",L"렌더링"}}},
        {L"Fechar", {{L"en",L"Close"},{L"es",L"Cerrar"},{L"fr",L"Fermer"},{L"de",L"Schließen"},{L"it",L"Chiudi"},{L"ru",L"Закрыть"},{L"zh",L"关闭"},{L"ja",L"閉じる"},{L"ko",L"닫기"}}},
        {L"Suavizar bordas (antialiasing)", {{L"en",L"Smooth edges (antialiasing)"},{L"es",L"Suavizar bordes (antialiasing)"},{L"fr",L"Lisser les bords (anticrénelage)"},{L"de",L"Kanten glätten (Antialiasing)"},{L"it",L"Smussa bordi (antialiasing)"},{L"ru",L"Сглаживание краёв"},{L"zh",L"平滑边缘（抗锯齿）"},{L"ja",L"エッジを滑らかにする"},{L"ko",L"가장자리 부드럽게(안티앨리어싱)"}}},
        {L"Escolher cor da seleção...", {{L"en",L"Choose selection color..."},{L"es",L"Elegir color de selección..."},{L"fr",L"Choisir la couleur de sélection..."},{L"de",L"Auswahlfarbe wählen..."},{L"it",L"Scegli colore selezione..."},{L"ru",L"Выбрать цвет выделения..."},{L"zh",L"选择选区颜色..."},{L"ja",L"選択色を選ぶ..."},{L"ko",L"선택 색상 선택..."}}},
        {L"Modo de seleção:", {{L"en",L"Selection mode:"},{L"es",L"Modo de selección:"},{L"fr",L"Mode de sélection :"},{L"de",L"Auswahlmodus:"},{L"it",L"Modalità selezione:"},{L"ru",L"Режим выделения:"},{L"zh",L"选择模式："},{L"ja",L"選択モード："},{L"ko",L"선택 모드:"}}},
        {L"Retangular", {{L"en",L"Rectangular"},{L"es",L"Rectangular"},{L"fr",L"Rectangulaire"},{L"de",L"Rechteckig"},{L"it",L"Rettangolare"},{L"ru",L"Прямоугольное"},{L"zh",L"矩形"},{L"ja",L"長方形"},{L"ko",L"직사각형"}}},
        {L"Desenhar área livre", {{L"en",L"Freehand area"},{L"es",L"Dibujar área libre"},{L"fr",L"Zone libre"},{L"de",L"Freihandbereich"},{L"it",L"Area a mano libera"},{L"ru",L"Свободная область"},{L"zh",L"自由绘制区域"},{L"ja",L"フリーハンド領域"},{L"ko",L"자유 영역"}}},
        {L"Importar áudio...", {{L"en",L"Import audio..."},{L"es",L"Importar audio..."},{L"fr",L"Importer audio..."},{L"de",L"Audio importieren..."},{L"it",L"Importa audio..."},{L"ru",L"Импортировать аудио..."},{L"zh",L"导入音频..."},{L"ja",L"音声をインポート..."},{L"ko",L"오디오 가져오기..."}}},
        {L"Ouvir ao mover a pinça", {{L"en",L"Scrub audio with playhead"},{L"es",L"Escuchar al mover el cabezal"},{L"fr",L"Écouter en déplaçant la tête de lecture"},{L"de",L"Beim Bewegen des Abspielkopfs hören"},{L"it",L"Ascolta spostando la testina"},{L"ru",L"Прослушивать при перемещении курсора"},{L"zh",L"拖动播放头时试听"},{L"ja",L"再生ヘッドを動かして試聴"},{L"ko",L"재생 헤드 이동 시 미리 듣기"}}},
        {L"Incluir áudio (MP4/MOV)", {{L"en",L"Include audio (MP4/MOV)"},{L"es",L"Incluir audio (MP4/MOV)"},{L"fr",L"Inclure l'audio (MP4/MOV)"},{L"de",L"Audio einschließen (MP4/MOV)"},{L"it",L"Includi audio (MP4/MOV)"},{L"ru",L"Добавить звук (MP4/MOV)"},{L"zh",L"包含音频 (MP4/MOV)"},{L"ja",L"音声を含める (MP4/MOV)"},{L"ko",L"오디오 포함 (MP4/MOV)"}}},
        {L"Música", {{L"en",L"Music"},{L"es",L"Música"},{L"fr",L"Musique"},{L"de",L"Musik"},{L"it",L"Musica"},{L"ru",L"Музыка"},{L"zh",L"音乐"},{L"ja",L"音楽"},{L"ko",L"음악"},{L"nl",L"Muziek"},{L"tr",L"Müzik"},{L"pl",L"Muzyka"},{L"id",L"Musik"}}},
        {L"Total", {{L"en",L"Total"},{L"es",L"Total"},{L"fr",L"Total"},{L"de",L"Gesamt"},{L"it",L"Totale"},{L"ru",L"Всего"},{L"zh",L"总计"},{L"ja",L"合計"},{L"ko",L"합계"},{L"nl",L"Totaal"},{L"tr",L"Toplam"},{L"pl",L"Razem"},{L"id",L"Total"}}},
        {L"quadro", {{L"en",L"frame"},{L"es",L"fotograma"},{L"fr",L"image"},{L"de",L"Einzelbild"},{L"it",L"fotogramma"},{L"ru",L"кадр"},{L"zh",L"帧"},{L"ja",L"フレーム"},{L"ko",L"프레임"},{L"nl",L"beeld"},{L"tr",L"kare"},{L"pl",L"klatka"},{L"id",L"bingkai"}}},
        {L"quadros", {{L"en",L"frames"},{L"es",L"fotogramas"},{L"fr",L"images"},{L"de",L"Einzelbilder"},{L"it",L"fotogrammi"},{L"ru",L"кадров"},{L"zh",L"帧"},{L"ja",L"フレーム"},{L"ko",L"프레임"},{L"nl",L"beelden"},{L"tr",L"kare"},{L"pl",L"klatki"},{L"id",L"bingkai"}}},
        {L"Renderizando animação", {{L"en",L"Rendering animation"},{L"es",L"Renderizando animación"},{L"fr",L"Rendu de l'animation"},{L"de",L"Animation wird gerendert"},{L"it",L"Rendering animazione"},{L"ru",L"Рендеринг анимации"},{L"zh",L"正在渲染动画"},{L"ja",L"アニメーションをレンダリング中"},{L"ko",L"애니메이션 렌더링 중"}}},
        {L"Kodanima — Renderização", {{L"en",L"Kodanima — Rendering"},{L"es",L"Kodanima — Renderización"},{L"fr",L"Kodanima — Rendu"},{L"de",L"Kodanima — Rendering"},{L"it",L"Kodanima — Rendering"},{L"ru",L"Kodanima — Рендеринг"},{L"zh",L"Kodanima — 渲染"},{L"ja",L"Kodanima — レンダリング"},{L"ko",L"Kodanima — 렌더링"}}},
        {L"Kodanima 2D - Canvas Principal (Ctrl+S Salva)", {{L"en",L"Kodanima 2D - Main Canvas (Ctrl+S Save)"},{L"es",L"Kodanima 2D - Lienzo principal (Ctrl+S Guardar)"},{L"fr",L"Kodanima 2D - Canevas principal (Ctrl+S Enregistrer)"},{L"de",L"Kodanima 2D - Hauptleinwand (Strg+S Speichern)"},{L"it",L"Kodanima 2D - Tela principale (Ctrl+S Salva)"},{L"ru",L"Kodanima 2D - Главный холст (Ctrl+S Сохранить)"},{L"zh",L"Kodanima 2D - 主画布 (Ctrl+S 保存)"},{L"ja",L"Kodanima 2D - メインキャンバス (Ctrl+S 保存)"},{L"ko",L"Kodanima 2D - 기본 캔버스 (Ctrl+S 저장)"}}},
        {L"PREFERÊNCIAS", {{L"en",L"PREFERENCES"},{L"es",L"PREFERENCIAS"},{L"fr",L"PRÉFÉRENCES"},{L"de",L"EINSTELLUNGEN"},{L"it",L"PREFERENZE"},{L"ru",L"НАСТРОЙКИ"},{L"zh",L"偏好设置"},{L"ja",L"設定"},{L"ko",L"환경설정"}}},
        {L"Lápis / pincéis", {{L"en",L"Pencil / brushes"},{L"es",L"Lápiz / pinceles"},{L"fr",L"Crayon / pinceaux"},{L"de",L"Stift / Pinsel"},{L"it",L"Matita / pennelli"},{L"ru",L"Карандаш / кисти"},{L"zh",L"铅笔 / 笔刷"},{L"ja",L"鉛筆 / ブラシ"},{L"ko",L"연필 / 브러시"}}},
        {L"Borracha", {{L"en",L"Eraser"},{L"es",L"Borrador"},{L"fr",L"Gomme"},{L"de",L"Radierer"},{L"it",L"Gomma"},{L"ru",L"Ластик"},{L"zh",L"橡皮擦"},{L"ja",L"消しゴム"},{L"ko",L"지우개"}}},
        {L"Balde de preenchimento", {{L"en",L"Fill bucket"},{L"es",L"Bote de pintura"},{L"fr",L"Pot de peinture"},{L"de",L"Fülleimer"},{L"it",L"Secchiello"},{L"ru",L"Заливка"},{L"zh",L"填充桶"},{L"ja",L"塗りつぶし"},{L"ko",L"채우기"}}},
        {L"Laço de seleção", {{L"en",L"Selection lasso"},{L"es",L"Lazo de selección"},{L"fr",L"Lasso de sélection"},{L"de",L"Auswahl-Lasso"},{L"it",L"Lazo di selezione"},{L"ru",L"Лассо выделения"},{L"zh",L"套索选择"},{L"ja",L"選択投げ縄"},{L"ko",L"선택 올가미"}}},
        {L"Escolher cor", {{L"en",L"Choose color"},{L"es",L"Elegir color"},{L"fr",L"Choisir une couleur"},{L"de",L"Farbe auswählen"},{L"it",L"Scegli colore"},{L"ru",L"Выбрать цвет"},{L"zh",L"选择颜色"},{L"ja",L"色を選択"},{L"ko",L"색상 선택"}}},
        {L"Conta-gotas", {{L"en",L"Eyedropper"},{L"es",L"Cuentagotas"},{L"fr",L"Pipette"},{L"de",L"Pipette"},{L"it",L"Contagocce"},{L"ru",L"Пипетка"},{L"zh",L"吸管"},{L"ja",L"スポイト"},{L"ko",L"스포이트"}}},
        {L"Renderizar animação", {{L"en",L"Render animation"},{L"es",L"Renderizar animación"},{L"fr",L"Rendre l'animation"},{L"de",L"Animation rendern"},{L"it",L"Renderizza animazione"},{L"ru",L"Рендерить анимацию"},{L"zh",L"渲染动画"},{L"ja",L"アニメーションをレンダリング"},{L"ko",L"애니메이션 렌더링"}}},
        {L"Brushes", {{L"en",L"Brushes"},{L"es",L"Pinceles"},{L"fr",L"Pinceaux"},{L"de",L"Pinsel"},{L"it",L"Pennelli"},{L"ru",L"Кисти"},{L"zh",L"笔刷"},{L"ja",L"ブラシ"},{L"ko",L"브러시"}}},
        {L"Tamanho", {{L"en",L"Size"},{L"es",L"Tamaño"},{L"fr",L"Taille"},{L"de",L"Größe"},{L"it",L"Dimensione"},{L"ru",L"Размер"},{L"zh",L"大小"},{L"ja",L"サイズ"},{L"ko",L"크기"}}},
        {L"Tamanho mínimo", {{L"en",L"Minimum size"},{L"es",L"Tamaño mínimo"},{L"fr",L"Taille minimale"},{L"de",L"Mindestgröße"},{L"it",L"Dimensione minima"},{L"ru",L"Минимальный размер"},{L"zh",L"最小尺寸"},{L"ja",L"最小サイズ"},{L"ko",L"최소 크기"}}},
        {L"Tamanho máximo", {{L"en",L"Maximum size"},{L"es",L"Tamaño máximo"},{L"fr",L"Taille maximale"},{L"de",L"Maximalgröße"},{L"it",L"Dimensione massima"},{L"ru",L"Максимальный размер"},{L"zh",L"最大尺寸"},{L"ja",L"最大サイズ"},{L"ko",L"최대 크기"}}},
        {L"Dureza (0–3)", {{L"en",L"Hardness (0–3)"},{L"es",L"Dureza (0–3)"},{L"fr",L"Dureté (0–3)"},{L"de",L"Härte (0–3)"},{L"it",L"Durezza (0–3)"},{L"ru",L"Жёсткость (0–3)"},{L"zh",L"硬度 (0–3)"},{L"ja",L"硬さ (0–3)"},{L"ko",L"경도 (0–3)"}}},
        {L"Redondez (%)", {{L"en",L"Roundness (%)"},{L"es",L"Redondez (%)"},{L"fr",L"Rondeur (%)"},{L"de",L"Rundheit (%)"},{L"it",L"Rotondità (%)"},{L"ru",L"Округлость (%)"},{L"zh",L"圆度 (%)"},{L"ja",L"丸み (%)"},{L"ko",L"둥글기 (%)"}}},
        {L"Espaçamento (%)", {{L"en",L"Spacing (%)"},{L"es",L"Espaciado (%)"},{L"fr",L"Espacement (%)"},{L"de",L"Abstand (%)"},{L"it",L"Spaziatura (%)"},{L"ru",L"Интервал (%)"},{L"zh",L"间距 (%)"},{L"ja",L"間隔 (%)"},{L"ko",L"간격 (%)"}}},
        {L"Opacidade (%)", {{L"en",L"Opacity (%)"},{L"es",L"Opacidad (%)"},{L"fr",L"Opacité (%)"},{L"de",L"Deckkraft (%)"},{L"it",L"Opacità (%)"},{L"ru",L"Непрозрачность (%)"},{L"zh",L"不透明度 (%)"},{L"ja",L"不透明度 (%)"},{L"ko",L"불투명도 (%)"}}},
        {L"Fluxo (%)", {{L"en",L"Flow (%)"},{L"es",L"Flujo (%)"},{L"fr",L"Flux (%)"},{L"de",L"Fluss (%)"},{L"it",L"Flusso (%)"},{L"ru",L"Поток (%)"},{L"zh",L"流量 (%)"},{L"ja",L"流量 (%)"},{L"ko",L"흐름 (%)"}}},
        {L"Dispersão básica (%)", {{L"en",L"Base scatter (%)"},{L"es",L"Dispersión básica (%)"},{L"fr",L"Dispersion de base (%)"},{L"de",L"Grundstreuung (%)"},{L"it",L"Dispersione base (%)"},{L"ru",L"Базовый разброс (%)"},{L"zh",L"基础散布 (%)"},{L"ja",L"基本散布 (%)"},{L"ko",L"기본 분산 (%)"}}},
        {L"Ângulo base (graus)", {{L"en",L"Base angle (degrees)"},{L"es",L"Ángulo base (grados)"},{L"fr",L"Angle de base (degrés)"},{L"de",L"Grundwinkel (Grad)"},{L"it",L"Angolo base (gradi)"},{L"ru",L"Базовый угол (градусы)"},{L"zh",L"基础角度（度）"},{L"ja",L"基本角度（度）"},{L"ko",L"기본 각도(도)"}}},
        {L"Escala aleatória (%)", {{L"en",L"Random scale (%)"},{L"es",L"Escala aleatoria (%)"},{L"fr",L"Échelle aléatoire (%)"},{L"de",L"Zufallsskalierung (%)"},{L"it",L"Scala casuale (%)"},{L"ru",L"Случайный масштаб (%)"},{L"zh",L"随机缩放 (%)"},{L"ja",L"ランダムスケール (%)"},{L"ko",L"무작위 크기 (%)"}}},
        {L"Blur da ponta (%)", {{L"en",L"Tip blur (%)"},{L"es",L"Desenfoque de punta (%)"},{L"fr",L"Flou de la pointe (%)"},{L"de",L"Spitzenunschärfe (%)"},{L"it",L"Sfocatura punta (%)"},{L"ru",L"Размытие кончика (%)"},{L"zh",L"笔尖模糊 (%)"},{L"ja",L"先端のぼかし (%)"},{L"ko",L"끝 흐림 (%)"}}},
        {L"Partículas por passo", {{L"en",L"Particles per step"},{L"es",L"Partículas por paso"},{L"fr",L"Particules par étape"},{L"de",L"Partikel pro Schritt"},{L"it",L"Particelle per passo"},{L"ru",L"Частиц на шаг"},{L"zh",L"每步粒子数"},{L"ja",L"ステップごとの粒子数"},{L"ko",L"단계별 입자"}}},
        {L"Dispersão de partículas (%)", {{L"en",L"Particle scatter (%)"},{L"es",L"Dispersión de partículas (%)"},{L"fr",L"Dispersion des particules (%)"},{L"de",L"Partikelstreuung (%)"},{L"it",L"Dispersione particelle (%)"},{L"ru",L"Разброс частиц (%)"},{L"zh",L"粒子散布 (%)"},{L"ja",L"パーティクル散布 (%)"},{L"ko",L"입자 분산 (%)"}}},
        {L"Densidade do spray", {{L"en",L"Spray density"},{L"es",L"Densidad del aerosol"},{L"fr",L"Densité du spray"},{L"de",L"Spraydichte"},{L"it",L"Densità spray"},{L"ru",L"Плотность распыления"},{L"zh",L"喷涂密度"},{L"ja",L"スプレー密度"},{L"ko",L"스프레이 밀도"}}},
        {L"Raio do spray (%)", {{L"en",L"Spray radius (%)"},{L"es",L"Radio del aerosol (%)"},{L"fr",L"Rayon du spray (%)"},{L"de",L"Sprayradius (%)"},{L"it",L"Raggio spray (%)"},{L"ru",L"Радиус распыления (%)"},{L"zh",L"喷涂半径 (%)"},{L"ja",L"スプレー半径 (%)"},{L"ko",L"스프레이 반경 (%)"}}},
        {L"Rotacionar acompanhando o traço", {{L"en",L"Rotate along stroke"},{L"es",L"Rotar siguiendo el trazo"},{L"fr",L"Rotation selon le trait"},{L"de",L"Entlang des Strichs drehen"},{L"it",L"Ruota seguendo il tratto"},{L"ru",L"Вращать по направлению штриха"},{L"zh",L"沿笔画方向旋转"},{L"ja",L"ストロークに沿って回転"},{L"ko",L"획 방향으로 회전"}}},
        {L"Rotação aleatória por carimbo", {{L"en",L"Random rotation per stamp"},{L"es",L"Rotación aleatoria por sello"},{L"fr",L"Rotation aléatoire par empreinte"},{L"de",L"Zufallsdrehung pro Stempel"},{L"it",L"Rotazione casuale per timbro"},{L"ru",L"Случайный поворот каждого отпечатка"},{L"zh",L"每个印章随机旋转"},{L"ja",L"スタンプごとのランダム回転"},{L"ko",L"도장별 무작위 회전"}}},
        {L"Seguir direção da caneta", {{L"en",L"Follow pen direction"},{L"es",L"Seguir la dirección del lápiz"},{L"fr",L"Suivre la direction du stylet"},{L"de",L"Stiftrichtung folgen"},{L"it",L"Segui direzione penna"},{L"ru",L"Следовать направлению пера"},{L"zh",L"跟随笔的方向"},{L"ja",L"ペンの方向に従う"},{L"ko",L"펜 방향 따르기"}}},
        {L"Ativar partículas", {{L"en",L"Enable particles"},{L"es",L"Activar partículas"},{L"fr",L"Activer les particules"},{L"de",L"Partikel aktivieren"},{L"it",L"Attiva particelle"},{L"ru",L"Включить частицы"},{L"zh",L"启用粒子"},{L"ja",L"パーティクルを有効化"},{L"ko",L"입자 활성화"}}},
        {L"Ativar Spraying Effect", {{L"en",L"Enable spray effect"},{L"es",L"Activar efecto de aerosol"},{L"fr",L"Activer l'effet spray"},{L"de",L"Sprayeffekt aktivieren"},{L"it",L"Attiva effetto spray"},{L"ru",L"Включить распыление"},{L"zh",L"启用喷涂效果"},{L"ja",L"スプレー効果を有効化"},{L"ko",L"스프레이 효과 활성화"}}},
        {L"Tamanho controlado pela pressão", {{L"en",L"Size controlled by pressure"},{L"es",L"Tamaño controlado por presión"},{L"fr",L"Taille contrôlée par la pression"},{L"de",L"Größe durch Druck steuern"},{L"it",L"Dimensione controllata dalla pressione"},{L"ru",L"Размер зависит от нажима"},{L"zh",L"由压力控制大小"},{L"ja",L"筆圧でサイズを制御"},{L"ko",L"압력으로 크기 제어"}}},
        {L"Opacidade controlada pela pressão", {{L"en",L"Opacity controlled by pressure"},{L"es",L"Opacidad controlada por presión"},{L"fr",L"Opacité contrôlée par la pression"},{L"de",L"Deckkraft durch Druck steuern"},{L"it",L"Opacità controllata dalla pressione"},{L"ru",L"Непрозрачность зависит от нажима"},{L"zh",L"由压力控制不透明度"},{L"ja",L"筆圧で不透明度を制御"},{L"ko",L"압력으로 불투명도 제어"}}},
        {L"Blur suaviza a máscara 3x3; escala e rotação variam de modo determinístico. Os botões abaixo abrem cada painel Sub Tool Detail sem perder as configurações MKV.", {{L"en",L"Blur smooths the 3x3 mask; scale and rotation vary deterministically. The buttons below open each Sub Tool Detail panel without losing MKV settings."},{L"es",L"El desenfoque suaviza la máscara 3x3; la escala y la rotación varían de forma determinista. Los botones abren los paneles Sub Tool Detail sin perder la configuración MKV."},{L"fr",L"Le flou adoucit le masque 3x3 ; l'échelle et la rotation varient de façon déterministe. Les boutons ouvrent les panneaux Sub Tool Detail sans perdre les réglages MKV."}}},
        {L"Camada de textura 2 — imagem BMP/PNG", {{L"en",L"Texture layer 2 — BMP/PNG image"},{L"es",L"Capa de textura 2 — imagen BMP/PNG"},{L"fr",L"Couche de texture 2 — image BMP/PNG"},{L"de",L"Texturebene 2 — BMP/PNG-Bild"},{L"it",L"Livello texture 2 — immagine BMP/PNG"},{L"ru",L"Текстура 2 — изображение BMP/PNG"},{L"zh",L"纹理层 2 — BMP/PNG 图像"},{L"ja",L"テクスチャレイヤー 2 — BMP/PNG 画像"},{L"ko",L"텍스처 레이어 2 — BMP/PNG 이미지"}}},
        {L"Camada de textura 3 — imagem BMP/PNG", {{L"en",L"Texture layer 3 — BMP/PNG image"},{L"es",L"Capa de textura 3 — imagen BMP/PNG"},{L"fr",L"Couche de texture 3 — image BMP/PNG"},{L"de",L"Texturebene 3 — BMP/PNG-Bild"},{L"it",L"Livello texture 3 — immagine BMP/PNG"},{L"ru",L"Текстура 3 — изображение BMP/PNG"},{L"zh",L"纹理层 3 — BMP/PNG 图像"},{L"ja",L"テクスチャレイヤー 3 — BMP/PNG 画像"},{L"ko",L"텍스처 레이어 3 — BMP/PNG 이미지"}}}
    };
    return t;
}


// Complemento focado nas janelas que não tinham suas legendas e itens traduzidos.
// Os textos de origem são estáveis para permitir alternar idiomas sem perder a chave.
static const std::map<std::wstring,std::map<std::wstring,std::wstring> >& KodaCatalogoUiSupplement() {
    static const std::map<std::wstring,std::map<std::wstring,std::wstring> > t = {
        {L"Interface", {{L"en",L"Interface"}, {L"es",L"Interfaz"}, {L"fr",L"Interface"}, {L"de",L"Oberfläche"}, {L"it",L"Interfaccia"}, {L"ru",L"Интерфейс"}, {L"zh",L"界面"}, {L"ja",L"インターフェース"}, {L"ko",L"인터페이스"}, {L"nl",L"Interface"}, {L"tr",L"Arayüz"}, {L"pl",L"Interfejs"}, {L"id",L"Antarmuka"}, {L"ar",L"الواجهة"}, {L"hi",L"इंटरफ़ेस"}}},
        {L"Selecione uma função e escolha a nova tecla. As alterações ficam ativas após clicar em Aplicar.", {{L"en",L"Select a function and choose the new key. Changes take effect after clicking Apply."}, {L"es",L"Seleccione una función y elija la nueva tecla. Los cambios se activan al pulsar Aplicar."}, {L"fr",L"Sélectionnez une fonction et choisissez la nouvelle touche. Les changements prennent effet après Appliquer."}, {L"de",L"Wählen Sie eine Funktion und eine neue Taste. Änderungen gelten nach Klick auf Übernehmen."}, {L"it",L"Seleziona una funzione e scegli il nuovo tasto. Le modifiche diventano attive dopo Applica."}, {L"ru",L"Выберите функцию и новую клавишу. Изменения вступят в силу после нажатия «Применить»."}, {L"zh",L"选择功能并指定新按键。单击“应用”后更改生效。"}, {L"ja",L"機能を選び、新しいキーを指定します。「適用」を押すと変更が有効になります。"}, {L"ko",L"기능을 선택하고 새 키를 지정하세요. 적용을 누르면 변경 사항이 활성화됩니다。"}}},
        {L"Funções e teclas", {{L"en",L"Functions and keys"}, {L"es",L"Funciones y teclas"}, {L"fr",L"Fonctions et touches"}, {L"de",L"Funktionen und Tasten"}, {L"it",L"Funzioni e tasti"}, {L"ru",L"Функции и клавиши"}, {L"zh",L"功能和按键"}, {L"ja",L"機能とキー"}, {L"ko",L"기능 및 키"}}},
        {L"Nova tecla", {{L"en",L"New key"}, {L"es",L"Nueva tecla"}, {L"fr",L"Nouvelle touche"}, {L"de",L"Neue Taste"}, {L"it",L"Nuovo tasto"}, {L"ru",L"Новая клавиша"}, {L"zh",L"新按键"}, {L"ja",L"新しいキー"}, {L"ko",L"새 키"}}},
        {L"Nova tecla:", {{L"en",L"New key:"}, {L"es",L"Nueva tecla:"}, {L"fr",L"Nouvelle touche :"}, {L"de",L"Neue Taste:"}, {L"it",L"Nuovo tasto:"}, {L"ru",L"Новая клавиша:"}, {L"zh",L"新按键："}, {L"ja",L"新しいキー："}, {L"ko",L"새 키:"}}},
        {L"Aplicar tecla", {{L"en",L"Apply key"}, {L"es",L"Aplicar tecla"}, {L"fr",L"Appliquer la touche"}, {L"de",L"Taste übernehmen"}, {L"it",L"Applica tasto"}, {L"ru",L"Применить клавишу"}, {L"zh",L"应用按键"}, {L"ja",L"キーを適用"}, {L"ko",L"키 적용"}}},
        {L"Salvar atalhos", {{L"en",L"Save shortcuts"}, {L"es",L"Guardar atajos"}, {L"fr",L"Enregistrer les raccourcis"}, {L"de",L"Tastenkürzel speichern"}, {L"it",L"Salva scorciatoie"}, {L"ru",L"Сохранить сочетания клавиш"}, {L"zh",L"保存快捷键"}, {L"ja",L"ショートカットを保存"}, {L"ko",L"단축키 저장"}}},
        {L"Restaurar padrão", {{L"en",L"Restore defaults"}, {L"es",L"Restaurar valores predeterminados"}, {L"fr",L"Restaurer les valeurs par défaut"}, {L"de",L"Standard wiederherstellen"}, {L"it",L"Ripristina predefiniti"}, {L"ru",L"Восстановить по умолчанию"}, {L"zh",L"恢复默认值"}, {L"ja",L"既定値に戻す"}, {L"ko",L"기본값 복원"}}},
        {L"Tema do programa", {{L"en",L"Application theme"}, {L"es",L"Tema de la aplicación"}, {L"fr",L"Thème de l’application"}, {L"de",L"Programmthema"}, {L"it",L"Tema del programma"}, {L"ru",L"Тема приложения"}, {L"zh",L"程序主题"}, {L"ja",L"アプリのテーマ"}, {L"ko",L"프로그램 테마"}}},
        {L"Escuro", {{L"en",L"Dark"}, {L"es",L"Oscuro"}, {L"fr",L"Sombre"}, {L"de",L"Dunkel"}, {L"it",L"Scuro"}, {L"ru",L"Тёмная"}, {L"zh",L"深色"}, {L"ja",L"ダーク"}, {L"ko",L"어둡게"}}},
        {L"Claro", {{L"en",L"Light"}, {L"es",L"Claro"}, {L"fr",L"Clair"}, {L"de",L"Hell"}, {L"it",L"Chiaro"}, {L"ru",L"Светлая"}, {L"zh",L"浅色"}, {L"ja",L"ライト"}, {L"ko",L"밝게"}}},
        {L"Pink", {{L"en",L"Pink"}, {L"es",L"Rosa"}, {L"fr",L"Rose"}, {L"de",L"Pink"}, {L"it",L"Rosa"}, {L"ru",L"Розовая"}, {L"zh",L"粉色"}, {L"ja",L"ピンク"}, {L"ko",L"핑크"}}},
        {L"Black com roxo", {{L"en",L"Black and purple"}, {L"es",L"Negro y morado"}, {L"fr",L"Noir et violet"}, {L"de",L"Schwarz und Lila"}, {L"it",L"Nero e viola"}, {L"ru",L"Чёрно-фиолетовая"}, {L"zh",L"黑紫色"}, {L"ja",L"黒と紫"}, {L"ko",L"검정과 보라"}}},
        {L"Tamanho da interface", {{L"en",L"Interface scale"}, {L"es",L"Tamaño de la interfaz"}, {L"fr",L"Échelle de l’interface"}, {L"de",L"Oberflächengröße"}, {L"it",L"Dimensione interfaccia"}, {L"ru",L"Масштаб интерфейса"}, {L"zh",L"界面缩放"}, {L"ja",L"インターフェース倍率"}, {L"ko",L"인터페이스 크기"}}},
        {L"Tamanho do texto", {{L"en",L"Text size"}, {L"es",L"Tamaño del texto"}, {L"fr",L"Taille du texte"}, {L"de",L"Textgröße"}, {L"it",L"Dimensione testo"}, {L"ru",L"Размер текста"}, {L"zh",L"文字大小"}, {L"ja",L"文字サイズ"}, {L"ko",L"텍스트 크기"}}},
        {L"Motor gráfico preferido", {{L"en",L"Preferred graphics engine"}, {L"es",L"Motor gráfico preferido"}, {L"fr",L"Moteur graphique préféré"}, {L"de",L"Bevorzugte Grafik-Engine"}, {L"it",L"Motore grafico preferito"}, {L"ru",L"Предпочтительный графический движок"}, {L"zh",L"首选图形引擎"}, {L"ja",L"優先グラフィックエンジン"}, {L"ko",L"선호 그래픽 엔진"}}},
        {L"Núcleos de CPU (0 = todos)", {{L"en",L"CPU cores (0 = all)"}, {L"es",L"Núcleos de CPU (0 = todos)"}, {L"fr",L"Cœurs CPU (0 = tous)"}, {L"de",L"CPU-Kerne (0 = alle)"}, {L"it",L"Core CPU (0 = tutti)"}, {L"ru",L"Ядра CPU (0 = все)"}, {L"zh",L"CPU 核心数（0 = 全部）"}, {L"ja",L"CPUコア数（0 = すべて）"}, {L"ko",L"CPU 코어(0 = 전체)"}}},
        {L"Todos os núcleos", {{L"en",L"All cores"}, {L"es",L"Todos los núcleos"}, {L"fr",L"Tous les cœurs"}, {L"de",L"Alle Kerne"}, {L"it",L"Tutti i core"}, {L"ru",L"Все ядра"}, {L"zh",L"所有核心"}, {L"ja",L"すべてのコア"}, {L"ko",L"모든 코어"}}},
        {L"Limite de RAM", {{L"en",L"RAM limit"}, {L"es",L"Límite de RAM"}, {L"fr",L"Limite de RAM"}, {L"de",L"RAM-Begrenzung"}, {L"it",L"Limite RAM"}, {L"ru",L"Лимит ОЗУ"}, {L"zh",L"内存限制"}, {L"ja",L"RAM上限"}, {L"ko",L"RAM 제한"}}},
        {L"CPU: aguardando detecção...", {{L"en",L"CPU: waiting for detection..."}, {L"es",L"CPU: esperando detección..."}, {L"fr",L"CPU : détection en attente..."}, {L"de",L"CPU: warte auf Erkennung..."}, {L"it",L"CPU: in attesa di rilevamento..."}, {L"ru",L"CPU: ожидание обнаружения..."}, {L"zh",L"CPU：等待检测..."}, {L"ja",L"CPU：検出待ち..."}, {L"ko",L"CPU: 감지 대기 중..."}}},
        {L"Recursos: --", {{L"en",L"Features: --"}, {L"es",L"Funciones: --"}, {L"fr",L"Fonctions : --"}, {L"de",L"Funktionen: --"}, {L"it",L"Funzioni: --"}, {L"ru",L"Возможности: --"}, {L"zh",L"功能：--"}, {L"ja",L"機能：--"}, {L"ko",L"기능: --"}}},
        {L"Kernel de pixels: --", {{L"en",L"Pixel kernel: --"}, {L"es",L"Kernel de píxeles: --"}, {L"fr",L"Noyau de pixels : --"}, {L"de",L"Pixel-Kernel: --"}, {L"it",L"Kernel pixel: --"}, {L"ru",L"Ядро пикселей: --"}, {L"zh",L"像素内核：--"}, {L"ja",L"ピクセルカーネル：--"}, {L"ko",L"픽셀 커널: --"}}},
        {L"API de pressão da caneta", {{L"en",L"Pen pressure API"}, {L"es",L"API de presión del lápiz"}, {L"fr",L"API de pression du stylet"}, {L"de",L"Stiftdruck-API"}, {L"it",L"API pressione penna"}, {L"ru",L"API давления пера"}, {L"zh",L"笔压 API"}, {L"ja",L"ペン圧力 API"}, {L"ko",L"펜 압력 API"}}},
        {L"Escolher cursor .cur/.ani", {{L"en",L"Choose cursor .cur/.ani"}, {L"es",L"Elegir cursor .cur/.ani"}, {L"fr",L"Choisir le curseur .cur/.ani"}, {L"de",L"Cursor .cur/.ani auswählen"}, {L"it",L"Scegli cursore .cur/.ani"}, {L"ru",L"Выбрать курсор .cur/.ani"}, {L"zh",L"选择 .cur/.ani 光标"}, {L"ja",L".cur/.ani カーソルを選択"}, {L"ko",L".cur/.ani 커서 선택"}}},
        {L"A API selecionada é usada para entrada da caneta digitalizadora.", {{L"en",L"The selected API is used for digitizer pen input."}, {L"es",L"La API seleccionada se usa para la entrada del lápiz digitalizador."}, {L"fr",L"L’API sélectionnée est utilisée pour la saisie au stylet."}, {L"de",L"Die ausgewählte API verarbeitet Stifteingaben des Tablets."}, {L"it",L"L’API selezionata gestisce l’input della penna della tavoletta."}, {L"ru",L"Выбранный API используется для ввода с пера планшета."}, {L"zh",L"所选 API 用于数位笔输入。"}, {L"ja",L"選択したAPIはペンタブレットの入力に使われます。"}, {L"ko",L"선택한 API는 태블릿 펜 입력에 사용됩니다."}}},
        {L"Mesa detectada:", {{L"en",L"Detected tablet:"}, {L"es",L"Tableta detectada:"}, {L"fr",L"Tablette détectée :"}, {L"de",L"Erkanntes Tablet:"}, {L"it",L"Tavoletta rilevata:"}, {L"ru",L"Обнаруженный планшет:"}, {L"zh",L"检测到的数位板："}, {L"ja",L"検出されたタブレット："}, {L"ko",L"감지된 태블릿:"}}},
        {L"Driver / versão:", {{L"en",L"Driver / version:"}, {L"es",L"Controlador / versión:"}, {L"fr",L"Pilote / version :"}, {L"de",L"Treiber / Version:"}, {L"it",L"Driver / versione:"}, {L"ru",L"Драйвер / версия:"}, {L"zh",L"驱动程序 / 版本："}, {L"ja",L"ドライバー / バージョン："}, {L"ko",L"드라이버 / 버전:"}}},
        {L"Consultando driver...", {{L"en",L"Checking driver..."}, {L"es",L"Consultando controlador..."}, {L"fr",L"Recherche du pilote..."}, {L"de",L"Treiber wird abgefragt..."}, {L"it",L"Verifica driver..."}, {L"ru",L"Проверка драйвера..."}, {L"zh",L"正在查询驱动程序..."}, {L"ja",L"ドライバーを確認中..."}, {L"ko",L"드라이버 확인 중..."}}},
        {L"Níveis de pressão:", {{L"en",L"Pressure levels:"}, {L"es",L"Niveles de presión:"}, {L"fr",L"Niveaux de pression :"}, {L"de",L"Druckstufen:"}, {L"it",L"Livelli di pressione:"}, {L"ru",L"Уровни давления:"}, {L"zh",L"压力级别："}, {L"ja",L"筆圧レベル："}, {L"ko",L"압력 단계:"}}},
        {L"Consultando eixo de pressão...", {{L"en",L"Checking pressure axis..."}, {L"es",L"Consultando eje de presión..."}, {L"fr",L"Recherche de l’axe de pression..."}, {L"de",L"Druckachse wird abgefragt..."}, {L"it",L"Verifica asse della pressione..."}, {L"ru",L"Проверка оси давления..."}, {L"zh",L"正在查询压力轴..."}, {L"ja",L"筆圧軸を確認中..."}, {L"ko",L"압력 축 확인 중..."}}},
        {L"Nome manual do perfil:", {{L"en",L"Manual profile name:"}, {L"es",L"Nombre manual del perfil:"}, {L"fr",L"Nom manuel du profil :"}, {L"de",L"Manueller Profilname:"}, {L"it",L"Nome profilo manuale:"}, {L"ru",L"Имя профиля вручную:"}, {L"zh",L"手动配置名称："}, {L"ja",L"手動プロファイル名："}, {L"ko",L"수동 프로필 이름:"}}},
        {L"Níveis manuais (0 = automático):", {{L"en",L"Manual levels (0 = automatic):"}, {L"es",L"Niveles manuales (0 = automático):"}, {L"fr",L"Niveaux manuels (0 = automatique) :"}, {L"de",L"Manuelle Stufen (0 = automatisch):"}, {L"it",L"Livelli manuali (0 = automatico):"}, {L"ru",L"Уровни вручную (0 = автоматически):"}, {L"zh",L"手动级别（0 = 自动）："}, {L"ja",L"手動レベル（0 = 自動）："}, {L"ko",L"수동 단계(0 = 자동):"}}},
        {L"Aplicar perfil", {{L"en",L"Apply profile"}, {L"es",L"Aplicar perfil"}, {L"fr",L"Appliquer le profil"}, {L"de",L"Profil übernehmen"}, {L"it",L"Applica profilo"}, {L"ru",L"Применить профиль"}, {L"zh",L"应用配置"}, {L"ja",L"プロファイルを適用"}, {L"ko",L"프로필 적용"}}},
        {L"Tamanho dos botões da timeline", {{L"en",L"Timeline button size"}, {L"es",L"Tamaño de botones de la línea de tiempo"}, {L"fr",L"Taille des boutons de la timeline"}, {L"de",L"Timeline-Schaltflächengröße"}, {L"it",L"Dimensione pulsanti timeline"}, {L"ru",L"Размер кнопок таймлайна"}, {L"zh",L"时间轴按钮大小"}, {L"ja",L"タイムラインボタンのサイズ"}, {L"ko",L"타임라인 버튼 크기"}}},
        {L"70% - compacto", {{L"en",L"70% - compact"}, {L"es",L"70% - compacto"}, {L"fr",L"70 % - compact"}, {L"de",L"70 % - kompakt"}, {L"it",L"70% - compatto"}, {L"ru",L"70% — компактный"}, {L"zh",L"70% - 紧凑"}, {L"ja",L"70% - コンパクト"}, {L"ko",L"70% - компакт"}}},
        {L"80% - pequeno", {{L"en",L"80% - small"}, {L"es",L"80% - pequeño"}, {L"fr",L"80 % - petit"}, {L"de",L"80 % - klein"}, {L"it",L"80% - piccolo"}, {L"ru",L"80% — маленький"}, {L"zh",L"80% - 小"}, {L"ja",L"80% - 小"}, {L"ko",L"80% - 작게"}}},
        {L"90% - médio", {{L"en",L"90% - medium"}, {L"es",L"90% - mediano"}, {L"fr",L"90 % - moyen"}, {L"de",L"90 % - mittel"}, {L"it",L"90% - medio"}, {L"ru",L"90% — средний"}, {L"zh",L"90% - 中"}, {L"ja",L"90% - 中"}, {L"ko",L"90% - 보통"}}},
        {L"100% - normal", {{L"en",L"100% - normal"}, {L"es",L"100% - normal"}, {L"fr",L"100 % - normal"}, {L"de",L"100 % - normal"}, {L"it",L"100% - normale"}, {L"ru",L"100% — обычный"}, {L"zh",L"100% - 正常"}, {L"ja",L"100% - 標準"}, {L"ko",L"100% - 보통"}}},
        {L"Altura das camadas", {{L"en",L"Layer height"}, {L"es",L"Altura de capas"}, {L"fr",L"Hauteur des calques"}, {L"de",L"Ebenenhöhe"}, {L"it",L"Altezza livelli"}, {L"ru",L"Высота слоёв"}, {L"zh",L"图层高度"}, {L"ja",L"レイヤーの高さ"}, {L"ko",L"레이어 높이"}}},
        {L"Altura da régua", {{L"en",L"Ruler height"}, {L"es",L"Altura de la regla"}, {L"fr",L"Hauteur de la règle"}, {L"de",L"Linealhöhe"}, {L"it",L"Altezza righello"}, {L"ru",L"Высота линейки"}, {L"zh",L"标尺高度"}, {L"ja",L"ルーラーの高さ"}, {L"ko",L"눈금자 높이"}}},
        {L"Desfazer / refazer", {{L"en",L"Undo / redo"}, {L"es",L"Deshacer / rehacer"}, {L"fr",L"Annuler / rétablir"}, {L"de",L"Rückgängig / Wiederholen"}, {L"it",L"Annulla / ripeti"}, {L"ru",L"Отменить / повторить"}, {L"zh",L"撤销 / 重做"}, {L"ja",L"元に戻す / やり直す"}, {L"ko",L"실행 취소 / 다시 실행"}}},
        {L"Limite de cache para o histórico", {{L"en",L"History cache limit"}, {L"es",L"Límite de caché del historial"}, {L"fr",L"Limite du cache de l’historique"}, {L"de",L"Verlauf-Cache-Limit"}, {L"it",L"Limite cache cronologia"}, {L"ru",L"Лимит кэша истории"}, {L"zh",L"历史记录缓存上限"}, {L"ja",L"履歴キャッシュ上限"}, {L"ko",L"기록 캐시 제한"}}},
        {L"O cache limita a memória usada para desfazer e refazer ações.", {{L"en",L"The cache limits memory used by undo and redo."}, {L"es",L"La caché limita la memoria usada para deshacer y rehacer."}, {L"fr",L"Le cache limite la mémoire utilisée pour annuler et rétablir."}, {L"de",L"Der Cache begrenzt den Speicher für Rückgängig/Wiederholen."}, {L"it",L"La cache limita la memoria usata da annulla/ripeti."}, {L"ru",L"Кэш ограничивает память для отмены и повтора действий."}, {L"zh",L"缓存限制撤销和重做操作占用的内存。"}, {L"ja",L"キャッシュは取り消し・やり直しに使うメモリを制限します。"}, {L"ko",L"캐시는 실행 취소 및 다시 실행에 사용되는 메모리를 제한합니다."}}},
        {L"Configuração salva em Documentos\\Kodanima\\config.ini", {{L"en",L"Settings saved in Documents\\Kodanima\\config.ini"}, {L"es",L"Configuración guardada en Documentos\\Kodanima\\config.ini"}, {L"fr",L"Configuration enregistrée dans Documents\\Kodanima\\config.ini"}, {L"de",L"Einstellungen gespeichert in Dokumente\\Kodanima\\config.ini"}, {L"it",L"Configurazione salvata in Documenti\\Kodanima\\config.ini"}, {L"ru",L"Настройки сохранены в Документы\\Kodanima\\config.ini"}, {L"zh",L"设置已保存到 Documents\\Kodanima\\config.ini"}, {L"ja",L"設定は Documents\\Kodanima\\config.ini に保存されます"}, {L"ko",L"설정은 Documents\\Kodanima\\config.ini에 저장됩니다"}}},
        {L"Salvar configurações", {{L"en",L"Save settings"}, {L"es",L"Guardar configuración"}, {L"fr",L"Enregistrer les paramètres"}, {L"de",L"Einstellungen speichern"}, {L"it",L"Salva impostazioni"}, {L"ru",L"Сохранить настройки"}, {L"zh",L"保存设置"}, {L"ja",L"設定を保存"}, {L"ko",L"설정 저장"}}},
        {L"Redefinir", {{L"en",L"Reset"}, {L"es",L"Restablecer"}, {L"fr",L"Réinitialiser"}, {L"de",L"Zurücksetzen"}, {L"it",L"Ripristina"}, {L"ru",L"Сбросить"}, {L"zh",L"重置"}, {L"ja",L"リセット"}, {L"ko",L"초기화"}}},
        {L"By creator Gabriel Freitas", {{L"en",L"By creator Gabriel Freitas"}, {L"es",L"Creado por Gabriel Freitas"}, {L"fr",L"Créé par Gabriel Freitas"}, {L"de",L"Erstellt von Gabriel Freitas"}, {L"it",L"Creato da Gabriel Freitas"}, {L"ru",L"Автор: Gabriel Freitas"}, {L"zh",L"作者 Gabriel Freitas"}, {L"ja",L"作者 Gabriel Freitas"}, {L"ko",L"제작자 Gabriel Freitas"}}},
        {L"⚙ Propriedades", {{L"en",L"⚙ Properties"}, {L"es",L"⚙ Propiedades"}, {L"fr",L"⚙ Propriétés"}, {L"de",L"⚙ Eigenschaften"}, {L"it",L"⚙ Proprietà"}, {L"ru",L"⚙ Свойства"}, {L"zh",L"⚙ 属性"}, {L"ja",L"⚙ プロパティ"}, {L"ko",L"⚙ 속성"}}},
        {L"Textura...", {{L"en",L"Texture..."}, {L"es",L"Textura..."}, {L"fr",L"Texture..."}, {L"de",L"Textur..."}, {L"it",L"Texture..."}, {L"ru",L"Текстура..."}, {L"zh",L"纹理..."}, {L"ja",L"テクスチャ..."}, {L"ko",L"텍스처..."}}},
        {L"Renomear", {{L"en",L"Rename"}, {L"es",L"Renombrar"}, {L"fr",L"Renommer"}, {L"de",L"Umbenennen"}, {L"it",L"Rinomina"}, {L"ru",L"Переименовать"}, {L"zh",L"重命名"}, {L"ja",L"名前を変更"}, {L"ko",L"이름 변경"}}},
        {L"Brush Tip — Ponta do pincel", {{L"en",L"Brush Tip — Brush tip"}, {L"es",L"Punta del pincel — Punta"}, {L"fr",L"Brush Tip — Pointe du pinceau"}, {L"de",L"Brush Tip — Pinselspitze"}, {L"it",L"Brush Tip — Punta pennello"}, {L"ru",L"Brush Tip — Кончик кисти"}, {L"zh",L"Brush Tip — 笔刷笔尖"}, {L"ja",L"Brush Tip — ブラシ先端"}, {L"ko",L"Brush Tip — 브러시 끝"}}},
        {L"Sub Tool Detail — Configurações do pincel", {{L"en",L"Sub Tool Detail — Brush settings"}, {L"es",L"Sub Tool Detail — Ajustes del pincel"}, {L"fr",L"Sub Tool Detail — Réglages du pinceau"}, {L"de",L"Sub Tool Detail — Pinseleinstellungen"}, {L"it",L"Sub Tool Detail — Impostazioni pennello"}, {L"ru",L"Sub Tool Detail — Настройки кисти"}, {L"zh",L"Sub Tool Detail — 笔刷设置"}, {L"ja",L"Sub Tool Detail — ブラシ設定"}, {L"ko",L"Sub Tool Detail — 브러시 설정"}}},
        {L"Stroke — Traçado", {{L"en",L"Stroke — Stroke"}, {L"es",L"Stroke — Trazo"}, {L"fr",L"Stroke — Tracé"}, {L"de",L"Stroke — Strich"}, {L"it",L"Stroke — Tratto"}, {L"ru",L"Stroke — Штрих"}, {L"zh",L"Stroke — 笔画"}, {L"ja",L"Stroke — ストローク"}, {L"ko",L"Stroke — 획"}}},
        {L"Texture — Texturas do pincel", {{L"en",L"Texture — Brush textures"}, {L"es",L"Texture — Texturas del pincel"}, {L"fr",L"Texture — Textures du pinceau"}, {L"de",L"Texture — Pinseltexturen"}, {L"it",L"Texture — Texture del pennello"}, {L"ru",L"Texture — Текстуры кисти"}, {L"zh",L"Texture — 笔刷纹理"}, {L"ja",L"Texture — ブラシテクスチャ"}, {L"ko",L"Texture — 브러시 텍스처"}}},
        {L"Spraying Effect — Pulverização", {{L"en",L"Spraying Effect — Spray"}, {L"es",L"Spraying Effect — Pulverización"}, {L"fr",L"Spraying Effect — Pulvérisation"}, {L"de",L"Spraying Effect — Sprühen"}, {L"it",L"Spraying Effect — Spruzzatura"}, {L"ru",L"Spraying Effect — Распыление"}, {L"zh",L"Spraying Effect — 喷涂"}, {L"ja",L"Spraying Effect — スプレー"}, {L"ko",L"Spraying Effect — 스프레이"}}},
        {L"Starting and Ending — Início e fim", {{L"en",L"Starting and Ending — Start and end"}, {L"es",L"Starting and Ending — Inicio y fin"}, {L"fr",L"Starting and Ending — Début et fin"}, {L"de",L"Starting and Ending — Anfang und Ende"}, {L"it",L"Starting and Ending — Inizio e fine"}, {L"ru",L"Starting and Ending — Начало и конец"}, {L"zh",L"Starting and Ending — 起始和结束"}, {L"ja",L"Starting and Ending — 開始と終了"}, {L"ko",L"Starting and Ending — 시작 및 끝"}}},
        {L"Ink / Pressure — Tinta e pressão", {{L"en",L"Ink / Pressure — Ink and pressure"}, {L"es",L"Ink / Pressure — Tinta y presión"}, {L"fr",L"Ink / Pressure — Encre et pression"}, {L"de",L"Ink / Pressure — Tinte und Druck"}, {L"it",L"Ink / Pressure — Inchiostro e pressione"}, {L"ru",L"Ink / Pressure — Чернила и нажим"}, {L"zh",L"Ink / Pressure — 墨水和压力"}, {L"ja",L"Ink / Pressure — インクと筆圧"}, {L"ko",L"Ink / Pressure — 잉크 및 압력"}}},
        {L"Anti-overflow e desempenho", {{L"en",L"Anti-overflow and performance"}, {L"es",L"Anti-overflow y rendimiento"}, {L"fr",L"Anti-débordement et performances"}, {L"de",L"Anti-Überlauf und Leistung"}, {L"it",L"Anti-overflow e prestazioni"}, {L"ru",L"Защита от переполнения и производительность"}, {L"zh",L"防溢出与性能"}, {L"ja",L"オーバーフロー防止と性能"}, {L"ko",L"오버플로 방지 및 성능"}}},
        {L"Mecânicas do brush (MKV)", {{L"en",L"Brush mechanics (MKV)"}, {L"es",L"Mecánicas del pincel (MKV)"}, {L"fr",L"Mécaniques du pinceau (MKV)"}, {L"de",L"Pinselmechaniken (MKV)"}, {L"it",L"Meccaniche del pennello (MKV)"}, {L"ru",L"Механики кисти (MKV)"}, {L"zh",L"笔刷机制 (MKV)"}, {L"ja",L"ブラシメカニクス (MKV)"}, {L"ko",L"브러시 메커니즘 (MKV)"}}},
        {L"Kodanima — Renderizar animação", {{L"en",L"Kodanima — Render animation"}, {L"es",L"Kodanima — Renderizar animación"}, {L"fr",L"Kodanima — Rendu de l’animation"}, {L"de",L"Kodanima — Animation rendern"}, {L"it",L"Kodanima — Render animazione"}, {L"ru",L"Kodanima — Рендер анимации"}, {L"zh",L"Kodanima — 渲染动画"}, {L"ja",L"Kodanima — アニメーションを書き出す"}, {L"ko",L"Kodanima — 애니메이션 렌더링"}}},
        {L"MP4 — H.264 (sem transparência)", {{L"en",L"MP4 — H.264 (no transparency)"}, {L"es",L"MP4 — H.264 (sin transparencia)"}, {L"fr",L"MP4 — H.264 (sans transparence)"}, {L"de",L"MP4 — H.264 (ohne Transparenz)"}, {L"it",L"MP4 — H.264 (senza trasparenza)"}, {L"ru",L"MP4 — H.264 (без прозрачности)"}, {L"zh",L"MP4 — H.264（无透明通道）"}, {L"ja",L"MP4 — H.264（透明度なし）"}, {L"ko",L"MP4 — H.264 (투명도 없음)"}}},
        {L"GIF animado", {{L"en",L"Animated GIF"}, {L"es",L"GIF animado"}, {L"fr",L"GIF animé"}, {L"de",L"Animiertes GIF"}, {L"it",L"GIF animata"}, {L"ru",L"Анимированный GIF"}, {L"zh",L"动画 GIF"}, {L"ja",L"アニメーションGIF"}, {L"ko",L"애니메이션 GIF"}}},
        {L"MOV — vídeo PNG (com transparência)", {{L"en",L"MOV — PNG video (with transparency)"}, {L"es",L"MOV — vídeo PNG (con transparencia)"}, {L"fr",L"MOV — vidéo PNG (avec transparence)"}, {L"de",L"MOV — PNG-Video (mit Transparenz)"}, {L"it",L"MOV — video PNG (con trasparenza)"}, {L"ru",L"MOV — видео PNG (с прозрачностью)"}, {L"zh",L"MOV — PNG 视频（带透明度）"}, {L"ja",L"MOV — PNG動画（透明度あり）"}, {L"ko",L"MOV — PNG 비디오(투명도 포함)"}}},
        {L"Fundo transparente (GIF/MOV)", {{L"en",L"Transparent background (GIF/MOV)"}, {L"es",L"Fondo transparente (GIF/MOV)"}, {L"fr",L"Arrière-plan transparent (GIF/MOV)"}, {L"de",L"Transparenter Hintergrund (GIF/MOV)"}, {L"it",L"Sfondo trasparente (GIF/MOV)"}, {L"ru",L"Прозрачный фон (GIF/MOV)"}, {L"zh",L"透明背景 (GIF/MOV)"}, {L"ja",L"透明な背景 (GIF/MOV)"}, {L"ko",L"투명 배경 (GIF/MOV)"}}},
        {L"MP4/H.264 exige Windows 7+; MOV/PNG pode incluir áudio PCM sem compressão.", {{L"en",L"MP4/H.264 requires Windows 7+; MOV/PNG can include uncompressed PCM audio."}, {L"es",L"MP4/H.264 requiere Windows 7+; MOV/PNG puede incluir audio PCM sin comprimir."}, {L"fr",L"MP4/H.264 nécessite Windows 7+ ; MOV/PNG peut inclure un audio PCM non compressé."}, {L"de",L"MP4/H.264 benötigt Windows 7+; MOV/PNG kann unkomprimiertes PCM-Audio enthalten."}, {L"it",L"MP4/H.264 richiede Windows 7+; MOV/PNG può includere audio PCM non compresso."}, {L"ru",L"Для MP4/H.264 нужна Windows 7+; MOV/PNG может включать несжатый звук PCM."}, {L"zh",L"MP4/H.264 需要 Windows 7+；MOV/PNG 可包含未压缩的 PCM 音频。"}, {L"ja",L"MP4/H.264にはWindows 7以降が必要です。MOV/PNGには非圧縮PCM音声を含められます。"}, {L"ko",L"MP4/H.264에는 Windows 7 이상이 필요합니다. MOV/PNG에는 압축되지 않은 PCM 오디오를 포함할 수 있습니다."}}},
        {L"CAMADAS / ORDEM", {{L"en",L"LAYERS / ORDER"}, {L"es",L"CAPAS / ORDEN"}, {L"fr",L"CALQUES / ORDRE"}, {L"de",L"EBENEN / REIHENFOLGE"}, {L"it",L"LIVELLI / ORDINE"}, {L"ru",L"СЛОИ / ПОРЯДОК"}, {L"zh",L"图层 / 顺序"}, {L"ja",L"レイヤー / 順序"}, {L"ko",L"레이어 / 순서"}}},
        {L"Total", {{L"en",L"Total"}, {L"es",L"Total"}, {L"fr",L"Total"}, {L"de",L"Gesamt"}, {L"it",L"Totale"}, {L"ru",L"Всего"}, {L"zh",L"总计"}, {L"ja",L"合計"}, {L"ko",L"합계"}}},
        {L"quadro", {{L"en",L"frame"}, {L"es",L"cuadro"}, {L"fr",L"image"}, {L"de",L"Frame"}, {L"it",L"fotogramma"}, {L"ru",L"кадр"}, {L"zh",L"帧"}, {L"ja",L"フレーム"}, {L"ko",L"프레임"}}},
        {L"quadros", {{L"en",L"frames"}, {L"es",L"cuadros"}, {L"fr",L"images"}, {L"de",L"Frames"}, {L"it",L"fotogrammi"}, {L"ru",L"кадры"}, {L"zh",L"帧"}, {L"ja",L"フレーム"}, {L"ko",L"프레임"}}},
        {L"Loop ON", {{L"en",L"Loop ON"}, {L"es",L"Bucle activado"}, {L"fr",L"Boucle activée"}, {L"de",L"Schleife EIN"}, {L"it",L"Loop attivo"}, {L"ru",L"Цикл вкл."}, {L"zh",L"循环开启"}, {L"ja",L"ループ ON"}, {L"ko",L"반복 켜짐"}}},
        {L"Loop", {{L"en",L"Loop"}, {L"es",L"Bucle"}, {L"fr",L"Boucle"}, {L"de",L"Schleife"}, {L"it",L"Loop"}, {L"ru",L"Цикл"}, {L"zh",L"循环"}, {L"ja",L"ループ"}, {L"ko",L"반복"}}},
        {L"Timeline: tema", {{L"en",L"Timeline: theme"}, {L"es",L"Línea de tiempo: tema"}, {L"fr",L"Timeline : thème"}, {L"de",L"Timeline: Thema"}, {L"it",L"Timeline: tema"}, {L"ru",L"Таймлайн: тема"}, {L"zh",L"时间轴：主题"}, {L"ja",L"タイムライン：テーマ"}, {L"ko",L"타임라인: 테마"}}},
        {L"Timeline: cor", {{L"en",L"Timeline: color"}, {L"es",L"Línea de tiempo: color"}, {L"fr",L"Timeline : couleur"}, {L"de",L"Timeline: Farbe"}, {L"it",L"Timeline: colore"}, {L"ru",L"Таймлайн: цвет"}, {L"zh",L"时间轴：颜色"}, {L"ja",L"タイムライン：色"}, {L"ko",L"타임라인: 색상"}}},
        {L"Suavizar bordas (antialiasing)", {{L"en",L"Smooth edges (antialiasing)"}, {L"es",L"Suavizar bordes (antialiasing)"}, {L"fr",L"Lisser les bords (anticrénelage)"}, {L"de",L"Kanten glätten (Antialiasing)"}, {L"it",L"Smussa bordi (antialiasing)"}, {L"ru",L"Сглаживание краёв (антиалиасинг)"}, {L"zh",L"平滑边缘（抗锯齿）"}, {L"ja",L"エッジを滑らかにする（アンチエイリアス）"}, {L"ko",L"가장자리 부드럽게(안티앨리어싱)"}}},
        {L"Estabilizador do traço", {{L"en",L"Stroke stabilizer"}, {L"es",L"Estabilizador del trazo"}, {L"fr",L"Stabilisateur du trait"}, {L"de",L"Strichstabilisator"}, {L"it",L"Stabilizzatore del tratto"}, {L"ru",L"Стабилизатор штриха"}, {L"zh",L"笔画稳定器"}, {L"ja",L"ストローク安定化"}, {L"ko",L"획 안정화"}}},
        {L"Força (0-100):", {{L"en",L"Strength (0–100):"}, {L"es",L"Fuerza (0–100):"}, {L"fr",L"Force (0–100) :"}, {L"de",L"Stärke (0–100):"}, {L"it",L"Forza (0–100):"}, {L"ru",L"Сила (0–100):"}, {L"zh",L"强度 (0–100)："}, {L"ja",L"強さ (0–100)："}, {L"ko",L"강도 (0–100):"}}},
        {L"Ouvir ao mover a pinça", {{L"en",L"Scrub audio with playhead"}, {L"es",L"Escuchar al mover el cabezal"}, {L"fr",L"Écouter en déplaçant la tête de lecture"}, {L"de",L"Beim Bewegen des Abspielkopfs hören"}, {L"it",L"Ascolta spostando la testina"}, {L"ru",L"Прослушивать при перемещении курсора"}, {L"zh",L"拖动播放头时试听"}, {L"ja",L"再生ヘッドを動かして試聴"}, {L"ko",L"재생 헤드 이동 시 미리 듣기"}}},
        {L"Propriedades do pincel", {{L"en",L"Brush properties"}, {L"es",L"Propiedades del pincel"}, {L"fr",L"Propriétés du pinceau"}, {L"de",L"Pinseleigenschaften"}, {L"it",L"Proprietà pennello"}, {L"ru",L"Свойства кисти"}, {L"zh",L"笔刷属性"}, {L"ja",L"ブラシのプロパティ"}, {L"ko",L"브러시 속성"}}},
        {L"Ponta BMP/PNG normalizada e salva na biblioteca do Kodanima. Os parâmetros e o vínculo da textura ficam no JSON mestre brushes.json; a imagem foi convertida para máscara de cobertura.", {{L"en",L"BMP/PNG brush tip normalized and saved to the Kodanima library. Parameters and texture references are stored in brushes.json; the image was converted to a coverage mask."}, {L"es",L"Punta BMP/PNG normalizada y guardada en la biblioteca de Kodanima. Los parámetros y texturas se guardan en brushes.json; la imagen se convirtió en una máscara de cobertura."}, {L"fr",L"Pointe BMP/PNG normalisée et enregistrée dans la bibliothèque Kodanima. Paramètres et textures sont dans brushes.json ; l’image a été convertie en masque de couverture."}, {L"de",L"BMP/PNG-Pinselspitze normalisiert und in der Kodanima-Bibliothek gespeichert. Parameter und Texturverweise stehen in brushes.json; das Bild wurde in eine Deckkraftmaske umgewandelt."}, {L"it",L"Punta BMP/PNG normalizzata e salvata nella libreria Kodanima. Parametri e texture sono in brushes.json; l’immagine è stata convertita in una maschera di copertura."}, {L"ru",L"Наконечник BMP/PNG нормализован и сохранён в библиотеке Kodanima. Параметры и текстуры находятся в brushes.json; изображение преобразовано в маску покрытия."}, {L"zh",L"BMP/PNG 笔尖已标准化并保存到 Kodanima 库。参数和纹理关联保存在 brushes.json；图像已转换为覆盖率蒙版。"}, {L"ja",L"BMP/PNGブラシ先端を正規化してKodanimaライブラリに保存しました。設定とテクスチャ参照はbrushes.jsonに保存され、画像はカバレッジマスクに変換されました。"}, {L"ko",L"BMP/PNG 브러시 끝을 정규화해 Kodanima 라이브러리에 저장했습니다. 매개변수와 텍스처 참조는 brushes.json에 저장되며 이미지는 커버리지 마스크로 변환되었습니다。"}}},
        {L"Nenhum pincel carregado.\nClique em + para importar um BMP/PNG.\nArquivos BMP válidos em AppData\\Roaming\\Pinceis do Kodanima também aparecem aqui.", {{L"en",L"No brush loaded.\nClick + to import a BMP/PNG.\nValid BMP files in AppData\\Roaming\\Pinceis do Kodanima also appear here."}, {L"es",L"No hay ningún pincel cargado.\nPulse + para importar un BMP/PNG.\nLos BMP válidos de AppData\\Roaming\\Pinceis do Kodanima también aparecen aquí."}, {L"fr",L"Aucun pinceau chargé.\nCliquez sur + pour importer un BMP/PNG.\nLes BMP valides d’AppData\\Roaming\\Pinceis do Kodanima apparaissent aussi ici."}, {L"de",L"Kein Pinsel geladen.\nKlicken Sie auf +, um BMP/PNG zu importieren.\nGültige BMP-Dateien in AppData\\Roaming\\Pinceis do Kodanima werden ebenfalls angezeigt."}, {L"it",L"Nessun pennello caricato.\nFai clic su + per importare BMP/PNG.\nAnche i BMP validi in AppData\\Roaming\\Pinceis do Kodanima vengono mostrati qui."}, {L"ru",L"Кисть не загружена.\nНажмите +, чтобы импортировать BMP/PNG.\nЗдесь также отображаются BMP из AppData\\Roaming\\Pinceis do Kodanima."}, {L"zh",L"未加载笔刷。\n点击 + 导入 BMP/PNG。\nAppData\\Roaming\\Pinceis do Kodanima 中的有效 BMP 也会显示在此处。"}, {L"ja",L"ブラシが読み込まれていません。\n+をクリックしてBMP/PNGをインポートします。\nAppData\\Roaming\\Pinceis do Kodanimaの有効なBMPもここに表示されます。"}, {L"ko",L"브러시가 로드되지 않았습니다.\n+를 클릭해 BMP/PNG를 가져오세요.\nAppData\\Roaming\\Pinceis do Kodanima의 유효한 BMP도 여기에 표시됩니다。"}}},
        {L"Grossura da borracha:", {{L"en",L"Eraser size:"}, {L"es",L"Grosor del borrador:"}, {L"fr",L"Taille de la gomme :"}, {L"de",L"Radierergröße:"}, {L"it",L"Dimensione gomma:"}, {L"ru",L"Размер ластика:"}, {L"zh",L"橡皮擦大小："}, {L"ja",L"消しゴムの太さ："}, {L"ko",L"지우개 크기:"}}},
        {L"Gap do balde (0-100):", {{L"en",L"Bucket gap (0–100):"}, {L"es",L"Separación del bote (0–100):"}, {L"fr",L"Écart du pot (0–100) :"}, {L"de",L"Fülllücken (0–100):"}, {L"it",L"Gap secchiello (0–100):"}, {L"ru",L"Зазор заливки (0–100):"}, {L"zh",L"油漆桶间隙 (0–100)："}, {L"ja",L"塗りつぶしの隙間 (0–100)："}, {L"ko",L"채우기 간격(0–100):"}}},
        {L"Tolerância (0-20 px):", {{L"en",L"Tolerance (0–20 px):"}, {L"es",L"Tolerancia (0–20 px):"}, {L"fr",L"Tolérance (0–20 px) :"}, {L"de",L"Toleranz (0–20 px):"}, {L"it",L"Tolleranza (0–20 px):"}, {L"ru",L"Допуск (0–20 пикс.):"}, {L"zh",L"容差 (0–20 px)："}, {L"ja",L"許容値 (0～20 px)："}, {L"ko",L"허용 오차(0–20px):"}}},
        {L"R = rotacionar | Shift = escala | Enter = confirmar", {{L"en",L"R = rotate | Shift = scale | Enter = confirm"}, {L"es",L"R = rotar | Shift = escala | Enter = confirmar"}, {L"fr",L"R = rotation | Maj = échelle | Entrée = confirmer"}, {L"de",L"R = drehen | Umschalt = skalieren | Eingabe = bestätigen"}, {L"it",L"R = ruota | Shift = scala | Invio = conferma"}, {L"ru",L"R = вращение | Shift = масштаб | Enter = подтвердить"}, {L"zh",L"R = 旋转 | Shift = 缩放 | Enter = 确认"}, {L"ja",L"R = 回転 | Shift = 拡大縮小 | Enter = 確定"}, {L"ko",L"R = 회전 | Shift = 크기 조절 | Enter = 확인"}}},
        {L"Grossura do lápis:", {{L"en",L"Pencil size:"}, {L"es",L"Grosor del lápiz:"}, {L"fr",L"Taille du crayon :"}, {L"de",L"Stiftgröße:"}, {L"it",L"Dimensione matita:"}, {L"ru",L"Размер карандаша:"}, {L"zh",L"铅笔粗细："}, {L"ja",L"鉛筆の太さ："}, {L"ko",L"연필 굵기:"}}},
        {L"Dureza do pincel:", {{L"en",L"Brush hardness:"}, {L"es",L"Dureza del pincel:"}, {L"fr",L"Dureté du pinceau :"}, {L"de",L"Pinselhärte:"}, {L"it",L"Durezza pennello:"}, {L"ru",L"Жёсткость кисти:"}, {L"zh",L"画笔硬度："}, {L"ja",L"ブラシの硬さ："}, {L"ko",L"브러시 경도:"}}},
        {L"Duro", {{L"en",L"Hard"}, {L"es",L"Duro"}, {L"fr",L"Dur"}, {L"de",L"Hart"}, {L"it",L"Duro"}, {L"ru",L"Жёсткий"}, {L"zh",L"硬"}, {L"ja",L"硬い"}, {L"ko",L"딱딱함"}}},
        {L"Suave", {{L"en",L"Soft"}, {L"es",L"Suave"}, {L"fr",L"Doux"}, {L"de",L"Weich"}, {L"it",L"Morbido"}, {L"ru",L"Мягкий"}, {L"zh",L"柔和"}, {L"ja",L"柔らかい"}, {L"ko",L"부드러움"}}},
        {L"Macio", {{L"en",L"Soft"}, {L"es",L"Blando"}, {L"fr",L"Doux"}, {L"de",L"Weich"}, {L"it",L"Morbido"}, {L"ru",L"Мягкий"}, {L"zh",L"软"}, {L"ja",L"柔らかい"}, {L"ko",L"부드러움"}}},
        {L"Firme", {{L"en",L"Firm"}, {L"es",L"Firme"}, {L"fr",L"Ferme"}, {L"de",L"Fest"}, {L"it",L"Duro"}, {L"ru",L"Плотный"}, {L"zh",L"稳固"}, {L"ja",L"しっかり"}, {L"ko",L"단단함"}}},
        {L"Espaçamento (%)", {{L"en",L"Spacing (%)"}, {L"es",L"Espaciado (%)"}, {L"fr",L"Espacement (%)"}, {L"de",L"Abstand (%)"}, {L"it",L"Spaziatura (%)"}, {L"ru",L"Интервал (%)"}, {L"zh",L"间距 (%)"}, {L"ja",L"間隔 (%)"}, {L"ko",L"간격 (%)"}}},
        {L"Opacidade (%)", {{L"en",L"Opacity (%)"}, {L"es",L"Opacidad (%)"}, {L"fr",L"Opacité (%)"}, {L"de",L"Deckkraft (%)"}, {L"it",L"Opacità (%)"}, {L"ru",L"Непрозрачность (%)"}, {L"zh",L"不透明度 (%)"}, {L"ja",L"不透明度 (%)"}, {L"ko",L"불투명도 (%)"}}},
        {L"Fluxo (%)", {{L"en",L"Flow (%)"}, {L"es",L"Flujo (%)"}, {L"fr",L"Flux (%)"}, {L"de",L"Fluss (%)"}, {L"it",L"Flusso (%)"}, {L"ru",L"Поток (%)"}, {L"zh",L"流量 (%)"}, {L"ja",L"フロー (%)"}, {L"ko",L"흐름 (%)"}}},
        {L"Dispersão básica (%)", {{L"en",L"Basic scatter (%)"}, {L"es",L"Dispersión básica (%)"}, {L"fr",L"Dispersion de base (%)"}, {L"de",L"Grundstreuung (%)"}, {L"it",L"Dispersione di base (%)"}, {L"ru",L"Базовый разброс (%)"}, {L"zh",L"基础散布 (%)"}, {L"ja",L"基本散布 (%)"}, {L"ko",L"기본 분산 (%)"}}},
        {L"O espaçamento define a distância entre as pontas repetidas ao longo do traço.", {{L"en",L"Spacing defines the distance between repeated brush tips along the stroke."}, {L"es",L"El espaciado define la distancia entre las puntas repetidas a lo largo del trazo."}, {L"fr",L"L’espacement définit la distance entre les pointes répétées le long du trait."}, {L"de",L"Der Abstand bestimmt den Abstand wiederholter Pinselspitzen entlang des Strichs."}, {L"it",L"La spaziatura definisce la distanza tra le punte ripetute lungo il tratto."}, {L"ru",L"Интервал определяет расстояние между отпечатками кисти вдоль штриха."}, {L"zh",L"间距决定沿笔画重复绘制笔尖之间的距离。"}, {L"ja",L"間隔はストロークに沿って繰り返されるブラシ先端の距離を指定します。"}, {L"ko",L"간격은 획을 따라 반복되는 브러시 끝 사이의 거리를 지정합니다."}}},
        {L"Escolher textura 2...", {{L"en",L"Choose texture 2..."}, {L"es",L"Elegir textura 2..."}, {L"fr",L"Choisir la texture 2..."}, {L"de",L"Textur 2 auswählen..."}, {L"it",L"Scegli texture 2..."}, {L"ru",L"Выбрать текстуру 2..."}, {L"zh",L"选择纹理 2..."}, {L"ja",L"テクスチャ2を選択..."}, {L"ko",L"텍스처 2 선택..."}}},
        {L"Tom da textura 2...", {{L"en",L"Tint texture 2..."}, {L"es",L"Tono de textura 2..."}, {L"fr",L"Teinte de texture 2..."}, {L"de",L"Farbton von Textur 2..."}, {L"it",L"Tinta texture 2..."}, {L"ru",L"Оттенок текстуры 2..."}, {L"zh",L"纹理 2 色调..."}, {L"ja",L"テクスチャ2の色調..."}, {L"ko",L"텍스처 2 색조..."}}},
        {L"Escolher textura 3...", {{L"en",L"Choose texture 3..."}, {L"es",L"Elegir textura 3..."}, {L"fr",L"Choisir la texture 3..."}, {L"de",L"Textur 3 auswählen..."}, {L"it",L"Scegli texture 3..."}, {L"ru",L"Выбрать текстуру 3..."}, {L"zh",L"选择纹理 3..."}, {L"ja",L"テクスチャ3を選択..."}, {L"ko",L"텍스처 3 선택..."}}},
        {L"Tom da textura 3...", {{L"en",L"Tint texture 3..."}, {L"es",L"Tono de textura 3..."}, {L"fr",L"Teinte de texture 3..."}, {L"de",L"Farbton von Textur 3..."}, {L"it",L"Tinta texture 3..."}, {L"ru",L"Оттенок текстуры 3..."}, {L"zh",L"纹理 3 色调..."}, {L"ja",L"テクスチャ3の色調..."}, {L"ko",L"텍스처 3 색조..."}}},
        {L"Camada de textura 2 — imagem BMP/PNG", {{L"en",L"Texture layer 2 — BMP/PNG image"}, {L"es",L"Capa de textura 2 — imagen BMP/PNG"}, {L"fr",L"Couche de texture 2 — image BMP/PNG"}, {L"de",L"Texturebene 2 — BMP/PNG-Bild"}, {L"it",L"Livello texture 2 — immagine BMP/PNG"}, {L"ru",L"Текстура 2 — изображение BMP/PNG"}, {L"zh",L"纹理层 2 — BMP/PNG 图像"}, {L"ja",L"テクスチャレイヤー 2 — BMP/PNG 画像"}, {L"ko",L"텍스처 레이어 2 — BMP/PNG 이미지"}}},
        {L"Camada de textura 3 — imagem BMP/PNG", {{L"en",L"Texture layer 3 — BMP/PNG image"}, {L"es",L"Capa de textura 3 — imagen BMP/PNG"}, {L"fr",L"Couche de texture 3 — image BMP/PNG"}, {L"de",L"Texturebene 3 — BMP/PNG-Bild"}, {L"it",L"Livello texture 3 — immagine BMP/PNG"}, {L"ru",L"Текстура 3 — изображение BMP/PNG"}, {L"zh",L"纹理层 3 — BMP/PNG 图像"}, {L"ja",L"テクスチャレイヤー 3 — BMP/PNG 画像"}, {L"ko",L"텍스처 레이어 3 — BMP/PNG 이미지"}}},
        {L"As camadas e tonalidades ficam vinculadas a este pincel no JSON mestre.", {{L"en",L"Layers and tints are linked to this brush in the master JSON."}, {L"es",L"Las capas y los tonos quedan vinculados a este pincel en el JSON maestro."}, {L"fr",L"Les couches et teintes sont liées à ce pinceau dans le JSON maître."}, {L"de",L"Ebenen und Farbtöne sind in der Master-JSON mit diesem Pinsel verknüpft."}, {L"it",L"Livelli e tonalità sono collegati a questo pennello nel JSON principale."}, {L"ru",L"Слои и оттенки связаны с этой кистью в основном JSON-файле."}, {L"zh",L"图层和色调会在主 JSON 中关联到此笔刷。"}, {L"ja",L"レイヤーと色調はマスターJSONでこのブラシに関連付けられます。"}, {L"ko",L"레이어와 색조는 마스터 JSON에서 이 브러시에 연결됩니다."}}},
        {L"Densidade do spray", {{L"en",L"Spray density"}, {L"es",L"Densidad del aerosol"}, {L"fr",L"Densité du spray"}, {L"de",L"Sprühdichte"}, {L"it",L"Densità spray"}, {L"ru",L"Плотность распыления"}, {L"zh",L"喷涂密度"}, {L"ja",L"スプレー密度"}, {L"ko",L"스프레이 밀도"}}},
        {L"Raio do spray (%)", {{L"en",L"Spray radius (%)"}, {L"es",L"Radio del aerosol (%)"}, {L"fr",L"Rayon du spray (%)"}, {L"de",L"Sprühradius (%)"}, {L"it",L"Raggio spray (%)"}, {L"ru",L"Радиус распыления (%)"}, {L"zh",L"喷涂半径 (%)"}, {L"ja",L"スプレー半径 (%)"}, {L"ko",L"스프레이 반경 (%)"}}},
        {L"A pulverização replica a ponta em posições pseudorrandômicas em torno do traço.", {{L"en",L"Spraying places brush tips at pseudorandom positions around the stroke."}, {L"es",L"La pulverización coloca puntas en posiciones seudoaleatorias alrededor del trazo."}, {L"fr",L"La pulvérisation place les pointes à des positions pseudoaléatoires autour du trait."}, {L"de",L"Der Sprüheffekt setzt Pinselspitzen an pseudozufälligen Positionen entlang des Strichs."}, {L"it",L"Lo spray colloca le punte in posizioni pseudocasuali attorno al tratto."}, {L"ru",L"Распыление размещает отпечатки кисти в псевдослучайных местах вокруг штриха."}, {L"zh",L"喷涂会在笔画周围的伪随机位置重复笔尖。"}, {L"ja",L"スプレーはストローク周囲の疑似ランダムな位置にブラシ先端を配置します。"}, {L"ko",L"스프레이는 획 주변의 의사 무작위 위치에 브러시 끝을 배치합니다."}}},
        {L"Tamanho inicial (%)", {{L"en",L"Start size (%)"}, {L"es",L"Tamaño inicial (%)"}, {L"fr",L"Taille initiale (%)"}, {L"de",L"Anfangsgröße (%)"}, {L"it",L"Dimensione iniziale (%)"}, {L"ru",L"Начальный размер (%)"}, {L"zh",L"初始大小 (%)"}, {L"ja",L"開始サイズ (%)"}, {L"ko",L"시작 크기 (%)"}}},
        {L"Tamanho final (%)", {{L"en",L"End size (%)"}, {L"es",L"Tamaño final (%)"}, {L"fr",L"Taille finale (%)"}, {L"de",L"Endgröße (%)"}, {L"it",L"Dimensione finale (%)"}, {L"ru",L"Конечный размер (%)"}, {L"zh",L"结束大小 (%)"}, {L"ja",L"終了サイズ (%)"}, {L"ko",L"끝 크기 (%)"}}},
        {L"Opacidade inicial (%)", {{L"en",L"Start opacity (%)"}, {L"es",L"Opacidad inicial (%)"}, {L"fr",L"Opacité initiale (%)"}, {L"de",L"Anfangsdeckkraft (%)"}, {L"it",L"Opacità iniziale (%)"}, {L"ru",L"Начальная непрозрачность (%)"}, {L"zh",L"初始不透明度 (%)"}, {L"ja",L"開始不透明度 (%)"}, {L"ko",L"시작 불투명도 (%)"}}},
        {L"Opacidade final (%)", {{L"en",L"End opacity (%)"}, {L"es",L"Opacidad final (%)"}, {L"fr",L"Opacité finale (%)"}, {L"de",L"Enddeckkraft (%)"}, {L"it",L"Opacità finale (%)"}, {L"ru",L"Конечная непрозрачность (%)"}, {L"zh",L"结束不透明度 (%)"}, {L"ja",L"終了不透明度 (%)"}, {L"ko",L"끝 불투명도 (%)"}}},
        {L"A pressão depende dos eventos recebidos da caneta/tablet; mouse normalmente fornece pressão binária.", {{L"en",L"Pressure depends on pen/tablet events; a mouse usually provides only binary pressure."}, {L"es",L"La presión depende de los eventos del lápiz/tableta; el ratón suele aportar presión binaria."}, {L"fr",L"La pression dépend des événements du stylet/de la tablette ; une souris fournit généralement une pression binaire."}, {L"de",L"Der Druck hängt von Stift-/Tablet-Ereignissen ab; eine Maus liefert normalerweise nur binären Druck."}, {L"it",L"La pressione dipende dagli eventi di penna/tavoletta; il mouse fornisce normalmente una pressione binaria."}, {L"ru",L"Давление зависит от событий пера/планшета; мышь обычно передаёт только двоичное давление."}, {L"zh",L"压力取决于笔/数位板事件；鼠标通常只提供二进制压力。"}, {L"ja",L"筆圧はペン/タブレットのイベントに依存し、マウスは通常二値の圧力しか提供しません。"}, {L"ko",L"압력은 펜/태블릿 이벤트에 따라 달라지며 마우스는 일반적으로 이진 압력만 제공합니다."}}},
        {L"Otimizar traço — aplicar ao soltar", {{L"en",L"Optimize stroke — apply on release"}, {L"es",L"Optimizar trazo — aplicar al soltar"}, {L"fr",L"Optimiser le trait — appliquer au relâchement"}, {L"de",L"Strich optimieren — beim Loslassen anwenden"}, {L"it",L"Ottimizza tratto — applica al rilascio"}, {L"ru",L"Оптимизация штриха — применить при отпускании"}, {L"zh",L"优化笔画 — 松开时应用"}, {L"ja",L"ストロークを最適化 — 離したときに適用"}, {L"ko",L"획 최적화 — 놓을 때 적용"}}},
        {L"Otimizar traço guarda segmentos durante a entrada e aplica o pincel ao terminar. Anti-overflow respeita a região de desenho.", {{L"en",L"Stroke optimization buffers segments during input and applies the brush at the end. Anti-overflow respects the drawing area."}, {L"es",L"La optimización guarda segmentos durante la entrada y aplica el pincel al finalizar. Anti-overflow respeta el área de dibujo."}, {L"fr",L"L’optimisation met les segments en mémoire pendant la saisie et applique le pinceau à la fin. Anti-overflow respecte la zone de dessin."}, {L"de",L"Die Strichoptimierung puffert Segmente während der Eingabe und wendet den Pinsel am Ende an. Anti-Overflow respektiert den Zeichenbereich."}, {L"it",L"L’ottimizzazione memorizza i segmenti durante l’input e applica il pennello alla fine. Anti-overflow rispetta l’area di disegno."}, {L"ru",L"Оптимизация сохраняет сегменты во время ввода и применяет кисть в конце. Защита от переполнения учитывает область рисования."}, {L"zh",L"笔画优化会在输入期间缓存线段，并在结束时应用笔刷。防溢出功能遵循绘制区域。"}, {L"ja",L"ストローク最適化は入力中にセグメントを保持し、終了時にブラシを適用します。オーバーフロー防止は描画領域を尊重します。"}, {L"ko",L"획 최적화는 입력 중 세그먼트를 저장하고 끝날 때 브러시를 적용합니다. 오버플로 방지는 그리기 영역을 따릅니다."}}},
        {L"Acesso rápido a todas as configurações Sub Tool Detail:", {{L"en",L"Quick access to all Sub Tool Detail settings:"}, {L"es",L"Acceso rápido a todos los ajustes de Sub Tool Detail:"}, {L"fr",L"Accès rapide à tous les réglages Sub Tool Detail :"}, {L"de",L"Schnellzugriff auf alle Sub-Tool-Detail-Einstellungen:"}, {L"it",L"Accesso rapido a tutte le impostazioni Sub Tool Detail:"}, {L"ru",L"Быстрый доступ ко всем настройкам Sub Tool Detail:"}, {L"zh",L"快速访问所有 Sub Tool Detail 设置："}, {L"ja",L"すべての Sub Tool Detail 設定へのクイックアクセス："}, {L"ko",L"모든 Sub Tool Detail 설정 빠른 액세스:"}}},
        {L"Configurações clássicas em janelas separadas; MKV contém as mecânicas próprias e atalhos para todos estes painéis.", {{L"en",L"Classic settings are in separate windows; MKV contains custom mechanics and shortcuts to all these panels."}, {L"es",L"Los ajustes clásicos están en ventanas separadas; MKV incluye mecánicas propias y accesos a todos estos paneles."}, {L"fr",L"Les réglages classiques sont dans des fenêtres séparées ; MKV contient ses mécaniques et des raccourcis vers tous ces panneaux."}, {L"de",L"Klassische Einstellungen befinden sich in separaten Fenstern; MKV enthält eigene Mechaniken und Verknüpfungen zu allen Panels."}, {L"it",L"Le impostazioni classiche sono in finestre separate; MKV contiene meccaniche dedicate e scorciatoie per tutti i pannelli."}, {L"ru",L"Классические настройки находятся в отдельных окнах; MKV содержит собственные механики и переходы ко всем панелям."}, {L"zh",L"经典设置位于独立窗口中；MKV 包含自定义机制以及所有这些面板的快捷入口。"}, {L"ja",L"従来の設定は個別ウィンドウにあり、MKVには独自の仕組みと各パネルへのショートカットがあります。"}, {L"ko",L"기본 설정은 별도 창에 있으며 MKV에는 전용 메커니즘과 각 패널 바로 가기가 있습니다."}}},
        {L"MKV reúne rotação, escala aleatória, partículas, dispersão e blur. As categorias à esquerda seguem a organização inspirada em Sub Tool Detail.", {{L"en",L"MKV combines rotation, random scale, particles, scatter and blur. Categories on the left follow a Sub Tool Detail-inspired layout."}, {L"es",L"MKV combina rotación, escala aleatoria, partículas, dispersión y desenfoque. Las categorías siguen una organización inspirada en Sub Tool Detail."}, {L"fr",L"MKV combine rotation, échelle aléatoire, particules, dispersion et flou. Les catégories s’inspirent de Sub Tool Detail."}, {L"de",L"MKV kombiniert Drehung, Zufallsskalierung, Partikel, Streuung und Weichzeichnung. Die Kategorien orientieren sich an Sub Tool Detail."}, {L"it",L"MKV combina rotazione, scala casuale, particelle, dispersione e sfocatura. Le categorie si ispirano a Sub Tool Detail."}, {L"ru",L"MKV объединяет вращение, случайный масштаб, частицы, разброс и размытие. Категории основаны на Sub Tool Detail."}, {L"zh",L"MKV 集成旋转、随机缩放、粒子、散布和模糊。分类布局参考 Sub Tool Detail。"}, {L"ja",L"MKV は回転、ランダム拡大縮小、パーティクル、散布、ぼかしを統合します。分類は Sub Tool Detail を参考にしています。"}, {L"ko",L"MKV는 회전, 임의 크기, 입자, 분산 및 흐림을 통합합니다. 범주는 Sub Tool Detail 형식을 참고합니다。"}}},
        {L"Incluir áudio (MP4/MOV)", {{L"en",L"Include audio (MP4/MOV)"}, {L"es",L"Incluir audio (MP4/MOV)"}, {L"fr",L"Inclure l’audio (MP4/MOV)"}, {L"de",L"Audio einschließen (MP4/MOV)"}, {L"it",L"Includi audio (MP4/MOV)"}, {L"ru",L"Добавить звук (MP4/MOV)"}, {L"zh",L"包含音频 (MP4/MOV)"}, {L"ja",L"音声を含める (MP4/MOV)"}, {L"ko",L"오디오 포함 (MP4/MOV)"}, {L"nl",L"Audio insluiten (MP4/MOV)"}, {L"tr",L"Sesi ekle (MP4/MOV)"}, {L"pl",L"Dołącz dźwięk (MP4/MOV)"}, {L"id",L"Sertakan audio (MP4/MOV)"}, {L"ar",L"تضمين الصوت (MP4/MOV)"}, {L"hi",L"ऑडियो शामिल करें (MP4/MOV)"}}},
        {L"Importar áudio...", {{L"en",L"Import audio..."}, {L"es",L"Importar audio..."}, {L"fr",L"Importer audio..."}, {L"de",L"Audio importieren..."}, {L"it",L"Importa audio..."}, {L"ru",L"Импортировать аудио..."}, {L"zh",L"导入音频..."}, {L"ja",L"音声をインポート..."}, {L"ko",L"오디오 가져오기..."}, {L"nl",L"Audio importeren..."}, {L"tr",L"Ses içe aktar..."}, {L"pl",L"Importuj dźwięk..."}, {L"id",L"Impor audio..."}, {L"ar",L"استيراد الصوت..."}, {L"hi",L"ऑडियो आयात करें..."}}},
        {L"Propriedades do balde", {{L"en",L"Bucket properties"}, {L"es",L"Propiedades del cubo"}, {L"fr",L"Propriétés du pot"}, {L"de",L"Fülleimer-Eigenschaften"}, {L"it",L"Proprietà secchiello"}, {L"ru",L"Свойства заливки"}, {L"zh",L"油漆桶属性"}, {L"ja",L"塗りつぶしのプロパティ"}, {L"ko",L"채우기 속성"}}},
        {L"Pincéis importados", {{L"en",L"Imported brushes"}, {L"es",L"Pinceles importados"}, {L"fr",L"Pinceaux importés"}, {L"de",L"Importierte Pinsel"}, {L"it",L"Pennelli importati"}, {L"ru",L"Импортированные кисти"}, {L"zh",L"导入的笔刷"}, {L"ja",L"読み込み済みブラシ"}, {L"ko",L"가져온 브러시"}}},
        {L"Prévia do pincel", {{L"en",L"Brush preview"}, {L"es",L"Vista previa del pincel"}, {L"fr",L"Aperçu du pinceau"}, {L"de",L"Pinselvorschau"}, {L"it",L"Anteprima pennello"}, {L"ru",L"Предпросмотр кисти"}, {L"zh",L"笔刷预览"}, {L"ja",L"ブラシプレビュー"}, {L"ko",L"브러시 미리보기"}}},
        {L"X Excluir", {{L"en",L"X Delete"}, {L"es",L"X Eliminar"}, {L"fr",L"X Supprimer"}, {L"de",L"X Löschen"}, {L"it",L"X Elimina"}, {L"ru",L"X Удалить"}, {L"zh",L"X 删除"}, {L"ja",L"X 削除"}, {L"ko",L"X 삭제"}}},
        {L"Pincéis importados: ATIVADOS (desmarque para usar o pincel padrão da engine)", {{L"en",L"Imported brushes: ON (uncheck to use the default engine brush)"}, {L"es",L"Pinceles importados: ACTIVADOS (desmarque para usar el pincel predeterminado)"}, {L"fr",L"Pinceaux importés : ACTIVÉS (décocher pour utiliser le pinceau par défaut)"}, {L"de",L"Importierte Pinsel: EIN (deaktivieren für Standardpinsel)"}, {L"it",L"Pennelli importati: ATTIVI (deseleziona per usare il pennello predefinito)"}, {L"ru",L"Импортированные кисти: ВКЛ. (снимите флажок для стандартной кисти)"}, {L"zh",L"导入笔刷：启用（取消勾选以使用引擎默认笔刷）"}, {L"ja",L"読み込みブラシ：オン（オフにすると既定ブラシを使用）"}, {L"ko",L"가져온 브러시: 켬(해제하면 기본 브러시 사용)"}}},
        {L"Brush Tip — forma e tamanho", {{L"en",L"Brush Tip — shape and size"}, {L"es",L"Punta del pincel — forma y tamaño"}, {L"fr",L"Pointe — forme et taille"}, {L"de",L"Pinselspitze — Form und Größe"}, {L"it",L"Punta pennello — forma e dimensione"}, {L"ru",L"Наконечник — форма и размер"}, {L"zh",L"笔尖 — 形状和大小"}, {L"ja",L"ブラシ先端 — 形状とサイズ"}, {L"ko",L"브러시 끝 — 모양 및 크기"}}},
        {L"Stroke — espaçamento e fluxo", {{L"en",L"Stroke — spacing and flow"}, {L"es",L"Trazo — espaciado y flujo"}, {L"fr",L"Trait — espacement et flux"}, {L"de",L"Strich — Abstand und Fluss"}, {L"it",L"Tratto — spaziatura e flusso"}, {L"ru",L"Штрих — интервал и поток"}, {L"zh",L"笔画 — 间距和流量"}, {L"ja",L"ストローク — 間隔と流量"}, {L"ko",L"획 — 간격 및 흐름"}}},
        {L"Texture — camadas de textura", {{L"en",L"Texture — texture layers"}, {L"es",L"Textura — capas de textura"}, {L"fr",L"Texture — couches de texture"}, {L"de",L"Textur — Texturebenen"}, {L"it",L"Texture — livelli texture"}, {L"ru",L"Текстура — слои текстуры"}, {L"zh",L"纹理 — 纹理图层"}, {L"ja",L"テクスチャ — テクスチャレイヤー"}, {L"ko",L"텍스처 — 텍스처 레이어"}}},
        {L"Spraying Effect — pulverização", {{L"en",L"Spraying Effect — spray"}, {L"es",L"Efecto de pulverización"}, {L"fr",L"Effet de pulvérisation"}, {L"de",L"Sprüh-Effekt"}, {L"it",L"Effetto spray"}, {L"ru",L"Распыление"}, {L"zh",L"喷涂效果"}, {L"ja",L"スプレー効果"}, {L"ko",L"스프레이 효과"}}},
        {L"Starting and Ending", {{L"en",L"Starting and Ending"}, {L"es",L"Inicio y final"}, {L"fr",L"Début et fin"}, {L"de",L"Anfang und Ende"}, {L"it",L"Inizio e fine"}, {L"ru",L"Начало и конец"}, {L"zh",L"起始与结束"}, {L"ja",L"開始と終了"}, {L"ko",L"시작 및 끝"}}},
        {L"Ink / Pressure — pressão", {{L"en",L"Ink / Pressure — pressure"}, {L"es",L"Tinta / presión"}, {L"fr",L"Encre / pression"}, {L"de",L"Tinte / Druck"}, {L"it",L"Inchiostro / pressione"}, {L"ru",L"Чернила / давление"}, {L"zh",L"墨水 / 压力"}, {L"ja",L"インク / 筆圧"}, {L"ko",L"잉크 / 압력"}}},
        {L"Mecânicas próprias (MKV)", {{L"en",L"Custom mechanics (MKV)"}, {L"es",L"Mecánicas propias (MKV)"}, {L"fr",L"Mécaniques personnalisées (MKV)"}, {L"de",L"Eigene Mechaniken (MKV)"}, {L"it",L"Meccaniche personalizzate (MKV)"}, {L"ru",L"Собственные механики (MKV)"}, {L"zh",L"自定义机制 (MKV)"}, {L"ja",L"独自の機構 (MKV)"}, {L"ko",L"사용자 지정 메커니즘 (MKV)"}}},
        {L"Selecione um pincel importado para editar as configurações.", {{L"en",L"Select an imported brush to edit its settings."}, {L"es",L"Seleccione un pincel importado para editar la configuración."}, {L"fr",L"Sélectionnez un pinceau importé pour modifier ses paramètres."}, {L"de",L"Wählen Sie einen importierten Pinsel zum Bearbeiten aus."}, {L"it",L"Seleziona un pennello importato per modificarne le impostazioni."}, {L"ru",L"Выберите импортированную кисть для изменения настроек."}, {L"zh",L"选择导入的笔刷以编辑其设置。"}, {L"ja",L"設定を編集する読み込み済みブラシを選択してください。"}, {L"ko",L"설정을 편집할 가져온 브러시를 선택하세요."}}},
        {L"Excluir este pincel importado?", {{L"en",L"Delete this imported brush?"}, {L"es",L"¿Eliminar este pincel importado?"}, {L"fr",L"Supprimer ce pinceau importé ?"}, {L"de",L"Diesen importierten Pinsel löschen?"}, {L"it",L"Eliminare questo pennello importato?"}, {L"ru",L"Удалить эту импортированную кисть?"}, {L"zh",L"删除此导入笔刷？"}, {L"ja",L"この読み込み済みブラシを削除しますか？"}, {L"ko",L"이 가져온 브러시를 삭제할까요?"}}},
        {L"Não foi possível carregar a máscara do pincel.", {{L"en",L"Could not load the brush mask."}, {L"es",L"No se pudo cargar la máscara del pincel."}, {L"fr",L"Impossible de charger le masque du pinceau."}, {L"de",L"Pinselmaske konnte nicht geladen werden."}, {L"it",L"Impossibile caricare la maschera del pennello."}, {L"ru",L"Не удалось загрузить маску кисти."}, {L"zh",L"无法加载笔刷蒙版。"}, {L"ja",L"ブラシマスクを読み込めませんでした。"}, {L"ko",L"브러시 마스크를 불러올 수 없습니다."}}},
        {L"Tamanho", {{L"en",L"Size"}, {L"es",L"Tamaño"}, {L"fr",L"Taille"}, {L"de",L"Größe"}, {L"it",L"Dimensione"}, {L"ru",L"Размер"}, {L"zh",L"大小"}, {L"ja",L"サイズ"}, {L"ko",L"크기"}}},
        {L"Tamanho mínimo", {{L"en",L"Minimum size"}, {L"es",L"Tamaño mínimo"}, {L"fr",L"Taille minimale"}, {L"de",L"Mindestgröße"}, {L"it",L"Dimensione minima"}, {L"ru",L"Минимальный размер"}, {L"zh",L"最小尺寸"}, {L"ja",L"最小サイズ"}, {L"ko",L"최소 크기"}}},
        {L"Tamanho máximo", {{L"en",L"Maximum size"}, {L"es",L"Tamaño máximo"}, {L"fr",L"Taille maximale"}, {L"de",L"Maximalgröße"}, {L"it",L"Dimensione massima"}, {L"ru",L"Максимальный размер"}, {L"zh",L"最大尺寸"}, {L"ja",L"最大サイズ"}, {L"ko",L"최대 크기"}}},
        {L"Dureza (0–3)", {{L"en",L"Hardness (0–3)"}, {L"es",L"Dureza (0–3)"}, {L"fr",L"Dureté (0–3)"}, {L"de",L"Härte (0–3)"}, {L"it",L"Durezza (0–3)"}, {L"ru",L"Жёсткость (0–3)"}, {L"zh",L"硬度 (0–3)"}, {L"ja",L"硬さ (0–3)"}, {L"ko",L"경도 (0–3)"}}},
        {L"Antialiasing da ponta", {{L"en",L"Tip antialiasing"}, {L"es",L"Suavizado de la punta"}, {L"fr",L"Anticrénelage de la pointe"}, {L"de",L"Kantenglättung der Spitze"}, {L"it",L"Antialiasing della punta"}, {L"ru",L"Сглаживание наконечника"}, {L"zh",L"笔尖抗锯齿"}, {L"ja",L"先端のアンチエイリアス"}, {L"ko",L"끝 안티앨리어싱"}}},
        {L"Ângulo base (graus)", {{L"en",L"Base angle (degrees)"}, {L"es",L"Ángulo base (grados)"}, {L"fr",L"Angle de base (degrés)"}, {L"de",L"Basiswinkel (Grad)"}, {L"it",L"Angolo base (gradi)"}, {L"ru",L"Базовый угол (градусы)"}, {L"zh",L"基础角度（度）"}, {L"ja",L"基本角度（度）"}, {L"ko",L"기본 각도(도)"}}},
        {L"Escala aleatória (%)", {{L"en",L"Random scale (%)"}, {L"es",L"Escala aleatoria (%)"}, {L"fr",L"Échelle aléatoire (%)"}, {L"de",L"Zufällige Skalierung (%)"}, {L"it",L"Scala casuale (%)"}, {L"ru",L"Случайный масштаб (%)"}, {L"zh",L"随机缩放 (%)"}, {L"ja",L"ランダム倍率 (%)"}, {L"ko",L"무작위 크기 (%)"}}},
        {L"Blur da ponta (%)", {{L"en",L"Tip blur (%)"}, {L"es",L"Desenfoque de punta (%)"}, {L"fr",L"Flou de pointe (%)"}, {L"de",L"Spitzenunschärfe (%)"}, {L"it",L"Sfocatura punta (%)"}, {L"ru",L"Размытие наконечника (%)"}, {L"zh",L"笔尖模糊 (%)"}, {L"ja",L"先端ぼかし (%)"}, {L"ko",L"끝 흐림 (%)"}}},
        {L"Partículas por passo", {{L"en",L"Particles per step"}, {L"es",L"Partículas por paso"}, {L"fr",L"Particules par étape"}, {L"de",L"Partikel pro Schritt"}, {L"it",L"Particelle per passaggio"}, {L"ru",L"Частиц на шаг"}, {L"zh",L"每步粒子数"}, {L"ja",L"ステップごとのパーティクル数"}, {L"ko",L"단계당 입자 수"}}},
        {L"Dispersão de partículas (%)", {{L"en",L"Particle spread (%)"}, {L"es",L"Dispersión de partículas (%)"}, {L"fr",L"Dispersion des particules (%)"}, {L"de",L"Partikelstreuung (%)"}, {L"it",L"Dispersione particelle (%)"}, {L"ru",L"Разброс частиц (%)"}, {L"zh",L"粒子散布 (%)"}, {L"ja",L"パーティクル散布 (%)"}, {L"ko",L"입자 분산 (%)"}}},
        {L"Dispersão base (%)", {{L"en",L"Base scatter (%)"}, {L"es",L"Dispersión base (%)"}, {L"fr",L"Dispersion de base (%)"}, {L"de",L"Basisstreuung (%)"}, {L"it",L"Dispersione base (%)"}, {L"ru",L"Базовый разброс (%)"}, {L"zh",L"基础散布 (%)"}, {L"ja",L"基本散布 (%)"}, {L"ko",L"기본 분산 (%)"}}},
        {L"Blur suaviza a máscara 3x3; escala e rotação variam de modo determinístico. Os botões abaixo abrem cada painel Sub Tool Detail sem perder as configurações MKV.", {{L"en",L"Blur smooths the 3×3 mask; scale and rotation vary deterministically. The buttons below open each Sub Tool Detail panel without losing MKV settings."}, {L"es",L"El desenfoque suaviza la máscara 3×3; la escala y la rotación varían de forma determinista. Los botones abren cada panel sin perder los ajustes MKV."}, {L"fr",L"Le flou adoucit le masque 3×3 ; l’échelle et la rotation varient de façon déterministe. Les boutons ouvrent chaque panneau sans perdre les réglages MKV."}, {L"de",L"Unschärfe glättet die 3×3-Maske; Skalierung und Drehung variieren deterministisch. Die Schaltflächen öffnen die Panels ohne MKV-Einstellungen zu verlieren."}, {L"it",L"La sfocatura ammorbidisce la maschera 3×3; scala e rotazione variano in modo deterministico. I pulsanti aprono i pannelli senza perdere le impostazioni MKV."}, {L"ru",L"Размытие сглаживает маску 3×3; масштаб и поворот меняются детерминированно. Кнопки открывают панели без потери настроек MKV."}, {L"zh",L"模糊会平滑 3×3 蒙版；缩放和旋转按确定性方式变化。下方按钮可打开各面板且保留 MKV 设置。"}, {L"ja",L"ぼかしは3×3マスクを滑らかにし、拡大縮小と回転は決定的に変化します。下のボタンでMKV設定を保ったまま各パネルを開きます。"}, {L"ko",L"흐림은 3×3 마스크를 부드럽게 하며 크기와 회전은 결정적으로 변합니다. 아래 버튼은 MKV 설정을 유지한 채 패널을 엽니다."}}},
        {L"Brush Tip", {{L"en",L"Brush Tip"}, {L"es",L"Punta del pincel"}, {L"fr",L"Pointe du pinceau"}, {L"de",L"Pinselspitze"}, {L"it",L"Punta pennello"}, {L"ru",L"Наконечник кисти"}, {L"zh",L"笔尖"}, {L"ja",L"ブラシ先端"}, {L"ko",L"브러시 끝"}}},
        {L"Stroke", {{L"en",L"Stroke"}, {L"es",L"Trazo"}, {L"fr",L"Trait"}, {L"de",L"Strich"}, {L"it",L"Tratto"}, {L"ru",L"Штрих"}, {L"zh",L"笔画"}, {L"ja",L"ストローク"}, {L"ko",L"획"}}},
        {L"Texture", {{L"en",L"Texture"}, {L"es",L"Textura"}, {L"fr",L"Texture"}, {L"de",L"Textur"}, {L"it",L"Texture"}, {L"ru",L"Текстура"}, {L"zh",L"纹理"}, {L"ja",L"テクスチャ"}, {L"ko",L"텍스처"}}},
        {L"Spray", {{L"en",L"Spray"}, {L"es",L"Spray"}, {L"fr",L"Pulvérisation"}, {L"de",L"Spray"}, {L"it",L"Spray"}, {L"ru",L"Распыление"}, {L"zh",L"喷涂"}, {L"ja",L"スプレー"}, {L"ko",L"스프레이"}}},
        {L"Start/End", {{L"en",L"Start/End"}, {L"es",L"Inicio/Final"}, {L"fr",L"Début/Fin"}, {L"de",L"Anfang/Ende"}, {L"it",L"Inizio/Fine"}, {L"ru",L"Начало/Конец"}, {L"zh",L"起始/结束"}, {L"ja",L"開始/終了"}, {L"ko",L"시작/끝"}}},
        {L"Ink/Pressão", {{L"en",L"Ink/Pressure"}, {L"es",L"Tinta/Presión"}, {L"fr",L"Encre/Pression"}, {L"de",L"Tinte/Druck"}, {L"it",L"Inchiostro/Pressione"}, {L"ru",L"Чернила/Давление"}, {L"zh",L"墨水/压力"}, {L"ja",L"インク/筆圧"}, {L"ko",L"잉크/압력"}}},
        {L"Anti-overflow", {{L"en",L"Anti-overflow"}, {L"es",L"Anti-desbordamiento"}, {L"fr",L"Anti-débordement"}, {L"de",L"Überlaufschutz"}, {L"it",L"Anti-overflow"}, {L"ru",L"Защита от выхода"}, {L"zh",L"防溢出"}, {L"ja",L"オーバーフロー防止"}, {L"ko",L"오버플로 방지"}}},
        {L"Diretório de saída:", {{L"en",L"Output directory:"}, {L"es",L"Directorio de salida:"}, {L"fr",L"Dossier de sortie :"}, {L"de",L"Ausgabeverzeichnis:"}, {L"it",L"Cartella di output:"}, {L"ru",L"Каталог вывода:"}, {L"zh",L"输出目录："}, {L"ja",L"出力ディレクトリ："}, {L"ko",L"출력 폴더:"}}},
        {L"Nome do arquivo:", {{L"en",L"File name:"}, {L"es",L"Nombre del archivo:"}, {L"fr",L"Nom du fichier :"}, {L"de",L"Dateiname:"}, {L"it",L"Nome file:"}, {L"ru",L"Имя файла:"}, {L"zh",L"文件名："}, {L"ja",L"ファイル名："}, {L"ko",L"파일 이름:"}}},
        {L"Formato de arquivo:", {{L"en",L"File format:"}, {L"es",L"Formato de archivo:"}, {L"fr",L"Format de fichier :"}, {L"de",L"Dateiformat:"}, {L"it",L"Formato file:"}, {L"ru",L"Формат файла:"}, {L"zh",L"文件格式："}, {L"ja",L"ファイル形式："}, {L"ko",L"파일 형식:"}}},
        {L"Procurar...", {{L"en",L"Browse..."}, {L"es",L"Examinar..."}, {L"fr",L"Parcourir..."}, {L"de",L"Durchsuchen..."}, {L"it",L"Sfoglia..."}, {L"ru",L"Обзор..."}, {L"zh",L"浏览..."}, {L"ja",L"参照..."}, {L"ko",L"찾아보기..."}}},
        {L"Renderizar", {{L"en",L"Render"}, {L"es",L"Renderizar"}, {L"fr",L"Rendu"}, {L"de",L"Rendern"}, {L"it",L"Renderizza"}, {L"ru",L"Рендер"}, {L"zh",L"渲染"}, {L"ja",L"レンダリング"}, {L"ko",L"렌더"}}},
        {L"Fechar", {{L"en",L"Close"}, {L"es",L"Cerrar"}, {L"fr",L"Fermer"}, {L"de",L"Schließen"}, {L"it",L"Chiudi"}, {L"ru",L"Закрыть"}, {L"zh",L"关闭"}, {L"ja",L"閉じる"}, {L"ko",L"닫기"}}},
        {L"PREFERÊNCIAS", {{L"en",L"PREFERENCES"}, {L"es",L"PREFERENCIAS"}, {L"fr",L"PRÉFÉRENCES"}, {L"de",L"EINSTELLUNGEN"}, {L"it",L"PREFERENZE"}, {L"ru",L"НАСТРОЙКИ"}, {L"zh",L"首选项"}, {L"ja",L"設定"}, {L"ko",L"환경설정"}}},
        {L"Roda de cores", {{L"en",L"Color wheel"}, {L"es",L"Rueda de colores"}, {L"fr",L"Roue chromatique"}, {L"de",L"Farbrad"}, {L"it",L"Ruota colori"}, {L"ru",L"Цветовой круг"}, {L"zh",L"色轮"}, {L"ja",L"カラーホイール"}, {L"ko",L"색상환"}}},
        {L"Escolher cor", {{L"en",L"Choose color"}, {L"es",L"Elegir color"}, {L"fr",L"Choisir une couleur"}, {L"de",L"Farbe auswählen"}, {L"it",L"Scegli colore"}, {L"ru",L"Выбрать цвет"}, {L"zh",L"选择颜色"}, {L"ja",L"色を選択"}, {L"ko",L"색상 선택"}}},
        {L"Música", {{L"en",L"Music"}, {L"es",L"Música"}, {L"fr",L"Musique"}, {L"de",L"Musik"}, {L"it",L"Musica"}, {L"ru",L"Музыка"}, {L"zh",L"音乐"}, {L"ja",L"音楽"}, {L"ko",L"음악"}}},
        {L"Play", {{L"en",L"Play"}, {L"es",L"Reproducir"}, {L"fr",L"Lecture"}, {L"de",L"Wiedergabe"}, {L"it",L"Riproduci"}, {L"ru",L"Воспроизвести"}, {L"zh",L"播放"}, {L"ja",L"再生"}, {L"ko",L"재생"}}},
        {L"Onion", {{L"en",L"Onion skin"}, {L"es",L"Papel cebolla"}, {L"fr",L"Pelure d’oignon"}, {L"de",L"Zwiebelschale"}, {L"it",L"Pelle di cipolla"}, {L"ru",L"Калька"}, {L"zh",L"洋葱皮"}, {L"ja",L"オニオンスキン"}, {L"ko",L"어니언 스킨"}}},
        {L"+ Camada", {{L"en",L"+ Layer"}, {L"es",L"+ Capa"}, {L"fr",L"+ Calque"}, {L"de",L"+ Ebene"}, {L"it",L"+ Livello"}, {L"ru",L"+ Слой"}, {L"zh",L"+ 图层"}, {L"ja",L"+ レイヤー"}, {L"ko",L"+ 레이어"}}},
        {L"Remover", {{L"en",L"Remove"}, {L"es",L"Quitar"}, {L"fr",L"Retirer"}, {L"de",L"Entfernen"}, {L"it",L"Rimuovi"}, {L"ru",L"Удалить"}, {L"zh",L"移除"}, {L"ja",L"削除"}, {L"ko",L"제거"}}},
        {L"+ Quadro", {{L"en",L"+ Frame"}, {L"es",L"+ Fotograma"}, {L"fr",L"+ Image"}, {L"de",L"+ Frame"}, {L"it",L"+ Fotogramma"}, {L"ru",L"+ Кадр"}, {L"zh",L"+ 帧"}, {L"ja",L"+ フレーム"}, {L"ko",L"+ 프레임"}}},
        {L"Inserir", {{L"en",L"Insert"}, {L"es",L"Insertar"}, {L"fr",L"Insérer"}, {L"de",L"Einfügen"}, {L"it",L"Inserisci"}, {L"ru",L"Вставить"}, {L"zh",L"插入"}, {L"ja",L"挿入"}, {L"ko",L"삽입"}}},
        {L"Excluir", {{L"en",L"Delete"}, {L"es",L"Eliminar"}, {L"fr",L"Supprimer"}, {L"de",L"Löschen"}, {L"it",L"Elimina"}, {L"ru",L"Удалить"}, {L"zh",L"删除"}, {L"ja",L"削除"}, {L"ko",L"삭제"}}},
        {L"Dur.", {{L"en",L"Dur."}, {L"es",L"Dur."}, {L"fr",L"Dur."}, {L"de",L"Dauer"}, {L"it",L"Dur."}, {L"ru",L"Длит."}, {L"zh",L"时长"}, {L"ja",L"長さ"}, {L"ko",L"시간"}}},
        {L"Kodanima | Timeline", {{L"en",L"Kodanima | Timeline"}, {L"es",L"Kodanima | Línea de tiempo"}, {L"fr",L"Kodanima | Chronologie"}, {L"de",L"Kodanima | Zeitleiste"}, {L"it",L"Kodanima | Timeline"}, {L"ru",L"Kodanima | Таймлайн"}, {L"zh",L"Kodanima | 时间轴"}, {L"ja",L"Kodanima | タイムライン"}, {L"ko",L"Kodanima | 타임라인"}}},
        {L"Kodanima — Renderização", {{L"en",L"Kodanima — Rendering"}, {L"es",L"Kodanima — Renderizado"}, {L"fr",L"Kodanima — Rendu"}, {L"de",L"Kodanima — Rendering"}, {L"it",L"Kodanima — Rendering"}, {L"ru",L"Kodanima — Рендер"}, {L"zh",L"Kodanima — 渲染"}, {L"ja",L"Kodanima — レンダリング"}, {L"ko",L"Kodanima — 렌더링"}}},
        {L"Tema", {{L"en",L"Theme"}, {L"es",L"Tema"}, {L"fr",L"Thème"}, {L"de",L"Thema"}, {L"it",L"Tema"}, {L"ru",L"Тема"}, {L"zh",L"主题"}, {L"ja",L"テーマ"}, {L"ko",L"테마"}}},
        {L"Cor tema", {{L"en",L"Theme color"}, {L"es",L"Color del tema"}, {L"fr",L"Couleur du thème"}, {L"de",L"Themenfarbe"}, {L"it",L"Colore tema"}, {L"ru",L"Цвет темы"}, {L"zh",L"主题颜色"}, {L"ja",L"テーマ色"}, {L"ko",L"테마 색상"}}},
        {L"Cor...", {{L"en",L"Color..."}, {L"es",L"Color..."}, {L"fr",L"Couleur..."}, {L"de",L"Farbe..."}, {L"it",L"Colore..."}, {L"ru",L"Цвет..."}, {L"zh",L"颜色..."}, {L"ja",L"色..."}, {L"ko",L"색상..."}}},
        {L"FPS", {{L"en",L"FPS"}, {L"es",L"FPS"}, {L"fr",L"IPS"}, {L"de",L"FPS"}, {L"it",L"FPS"}, {L"ru",L"Кадр/с"}, {L"zh",L"帧率"}, {L"ja",L"FPS"}, {L"ko",L"FPS"}}},
        {L"s", {{L"en",L"s"}, {L"es",L"s"}, {L"fr",L"s"}, {L"de",L"s"}, {L"it",L"s"}, {L"ru",L"с"}, {L"zh",L"秒"}, {L"ja",L"秒"}, {L"ko",L"초"}}},
        {L"■ Parar", {{L"en",L"■ Stop"}, {L"es",L"■ Detener"}, {L"fr",L"■ Arrêter"}, {L"de",L"■ Stoppen"}, {L"it",L"■ Ferma"}, {L"ru",L"■ Стоп"}, {L"zh",L"■ 停止"}, {L"ja",L"■ 停止"}, {L"ko",L"■ 정지"}}},
        {L"▶ Play", {{L"en",L"▶ Play"}, {L"es",L"▶ Reproducir"}, {L"fr",L"▶ Lecture"}, {L"de",L"▶ Wiedergabe"}, {L"it",L"▶ Riproduci"}, {L"ru",L"▶ Воспроизвести"}, {L"zh",L"▶ 播放"}, {L"ja",L"▶ 再生"}, {L"ko",L"▶ 재생"}}},
        {L"Importar áudio para a timeline", {{L"en",L"Import audio to timeline"}, {L"es",L"Importar audio a la línea de tiempo"}, {L"fr",L"Importer l’audio dans la timeline"}, {L"de",L"Audio in die Zeitleiste importieren"}, {L"it",L"Importa audio nella timeline"}, {L"ru",L"Импорт аудио на таймлайн"}, {L"zh",L"将音频导入时间轴"}, {L"ja",L"タイムラインに音声をインポート"}, {L"ko",L"타임라인에 오디오 가져오기"}}},
        {L"Escolher segunda textura", {{L"en",L"Choose second texture"}, {L"es",L"Elegir segunda textura"}, {L"fr",L"Choisir la deuxième texture"}, {L"de",L"Zweite Textur auswählen"}, {L"it",L"Scegli seconda texture"}, {L"ru",L"Выбрать вторую текстуру"}, {L"zh",L"选择第二个纹理"}, {L"ja",L"2つ目のテクスチャを選択"}, {L"ko",L"두 번째 텍스처 선택"}}},
        {L"Escolher terceira textura", {{L"en",L"Choose third texture"}, {L"es",L"Elegir tercera textura"}, {L"fr",L"Choisir la troisième texture"}, {L"de",L"Dritte Textur auswählen"}, {L"it",L"Scegli terza texture"}, {L"ru",L"Выбрать третью текстуру"}, {L"zh",L"选择第三个纹理"}, {L"ja",L"3つ目のテクスチャを選択"}, {L"ko",L"세 번째 텍스처 선택"}}},
        {L"Ocultar camada", {{L"en",L"Hide layer"}, {L"es",L"Ocultar capa"}, {L"fr",L"Masquer le calque"}, {L"de",L"Ebene ausblenden"}, {L"it",L"Nascondi livello"}, {L"ru",L"Скрыть слой"}, {L"zh",L"隐藏图层"}, {L"ja",L"レイヤーを非表示"}, {L"ko",L"레이어 숨기기"}}},
        {L"Mostrar camada", {{L"en",L"Show layer"}, {L"es",L"Mostrar capa"}, {L"fr",L"Afficher le calque"}, {L"de",L"Ebene anzeigen"}, {L"it",L"Mostra livello"}, {L"ru",L"Показать слой"}, {L"zh",L"显示图层"}, {L"ja",L"レイヤーを表示"}, {L"ko",L"레이어 표시"}}},
        {L"Renomear camada...", {{L"en",L"Rename layer..."}, {L"es",L"Renombrar capa..."}, {L"fr",L"Renommer le calque..."}, {L"de",L"Ebene umbenennen..."}, {L"it",L"Rinomina livello..."}, {L"ru",L"Переименовать слой..."}, {L"zh",L"重命名图层..."}, {L"ja",L"レイヤー名を変更..."}, {L"ko",L"레이어 이름 변경..."}}},
        {L"Onion skin nesta camada", {{L"en",L"Onion skin on this layer"}, {L"es",L"Papel cebolla en esta capa"}, {L"fr",L"Pelure d’oignon sur ce calque"}, {L"de",L"Zwiebelschale auf dieser Ebene"}, {L"it",L"Onion skin su questo livello"}, {L"ru",L"Просмотр соседних кадров на слое"}, {L"zh",L"此图层洋葱皮"}, {L"ja",L"このレイヤーのオニオンスキン"}, {L"ko",L"이 레이어 어니언 스킨"}}},
        {L"Criar quadro nesta camada", {{L"en",L"Create frame on this layer"}, {L"es",L"Crear fotograma en esta capa"}, {L"fr",L"Créer une image sur ce calque"}, {L"de",L"Frame auf dieser Ebene erstellen"}, {L"it",L"Crea fotogramma su questo livello"}, {L"ru",L"Создать кадр на этом слое"}, {L"zh",L"在此图层创建帧"}, {L"ja",L"このレイヤーにフレームを作成"}, {L"ko",L"이 레이어에 프레임 만들기"}}},
        {L"Abrir propriedades", {{L"en",L"Open properties"}, {L"es",L"Abrir propiedades"}, {L"fr",L"Ouvrir les propriétés"}, {L"de",L"Eigenschaften öffnen"}, {L"it",L"Apri proprietà"}, {L"ru",L"Открыть свойства"}, {L"zh",L"打开属性"}, {L"ja",L"プロパティを開く"}, {L"ko",L"속성 열기"}}},
        {L"Cores", {{L"en",L"Colors"}, {L"es",L"Colores"}, {L"fr",L"Couleurs"}, {L"de",L"Farben"}, {L"it",L"Colori"}, {L"ru",L"Цвета"}, {L"zh",L"颜色"}, {L"ja",L"色"}, {L"ko",L"색상"}}},
        {L"Cores básicas:", {{L"en",L"Basic colors:"}, {L"es",L"Colores básicos:"}, {L"fr",L"Couleurs de base :"}, {L"de",L"Grundfarben:"}, {L"it",L"Colori di base:"}, {L"ru",L"Основные цвета:"}, {L"zh",L"基本颜色："}, {L"ja",L"基本色："}, {L"ko",L"기본 색상:"}}},
        {L"Cores personalizadas:", {{L"en",L"Custom colors:"}, {L"es",L"Colores personalizados:"}, {L"fr",L"Couleurs personnalisées :"}, {L"de",L"Benutzerdefinierte Farben:"}, {L"it",L"Colori personalizzati:"}, {L"ru",L"Пользовательские цвета:"}, {L"zh",L"自定义颜色："}, {L"ja",L"カスタム色："}, {L"ko",L"사용자 지정 색상:"}}},
        {L"Definir cores personalizadas >>", {{L"en",L"Define Custom Colors >>"}, {L"es",L"Definir colores personalizados >>"}, {L"fr",L"Définir les couleurs personnalisées >>"}, {L"de",L"Benutzerdefinierte Farben festlegen >>"}, {L"it",L"Definisci colori personalizzati >>"}, {L"ru",L"Задать пользовательские цвета >>"}, {L"zh",L"定义自定义颜色 >>"}, {L"ja",L"カスタム色を定義 >>"}, {L"ko",L"사용자 지정 색상 정의 >>"}}},
        {L"Adicionar às cores personalizadas", {{L"en",L"Add to Custom Colors"}, {L"es",L"Añadir a colores personalizados"}, {L"fr",L"Ajouter aux couleurs personnalisées"}, {L"de",L"Zu benutzerdefinierten Farben hinzufügen"}, {L"it",L"Aggiungi ai colori personalizzati"}, {L"ru",L"Добавить в пользовательские цвета"}, {L"zh",L"添加到自定义颜色"}, {L"ja",L"カスタム色に追加"}, {L"ko",L"사용자 지정 색상에 추가"}}},
        {L"Cor sólida", {{L"en",L"Solid color"}, {L"es",L"Color sólido"}, {L"fr",L"Couleur unie"}, {L"de",L"Volltonfarbe"}, {L"it",L"Colore pieno"}, {L"ru",L"Сплошной цвет"}, {L"zh",L"纯色"}, {L"ja",L"単色"}, {L"ko",L"단색"}}},
        {L"Matiz:", {{L"en",L"Hue:"}, {L"es",L"Matiz:"}, {L"fr",L"Teinte :"}, {L"de",L"Farbton:"}, {L"it",L"Tonalità:"}, {L"ru",L"Оттенок:"}, {L"zh",L"色相："}, {L"ja",L"色相："}, {L"ko",L"색조:"}}},
        {L"Saturação:", {{L"en",L"Saturation:"}, {L"es",L"Saturación:"}, {L"fr",L"Saturation :"}, {L"de",L"Sättigung:"}, {L"it",L"Saturazione:"}, {L"ru",L"Насыщенность:"}, {L"zh",L"饱和度："}, {L"ja",L"彩度："}, {L"ko",L"채도:"}}},
        {L"Luminosidade:", {{L"en",L"Luminosity:"}, {L"es",L"Luminosidad:"}, {L"fr",L"Luminosité :"}, {L"de",L"Helligkeit:"}, {L"it",L"Luminosità:"}, {L"ru",L"Яркость:"}, {L"zh",L"亮度："}, {L"ja",L"明度："}, {L"ko",L"명도:"}}},
        {L"Vermelho:", {{L"en",L"Red:"}, {L"es",L"Rojo:"}, {L"fr",L"Rouge :"}, {L"de",L"Rot:"}, {L"it",L"Rosso:"}, {L"ru",L"Красный:"}, {L"zh",L"红色："}, {L"ja",L"赤："}, {L"ko",L"빨강:"}}},
        {L"Verde:", {{L"en",L"Green:"}, {L"es",L"Verde:"}, {L"fr",L"Vert :"}, {L"de",L"Grün:"}, {L"it",L"Verde:"}, {L"ru",L"Зелёный:"}, {L"zh",L"绿色："}, {L"ja",L"緑："}, {L"ko",L"초록:"}}},
        {L"Azul:", {{L"en",L"Blue:"}, {L"es",L"Azul:"}, {L"fr",L"Bleu :"}, {L"de",L"Blau:"}, {L"it",L"Blu:"}, {L"ru",L"Синий:"}, {L"zh",L"蓝色："}, {L"ja",L"青："}, {L"ko",L"파랑:"}}},
        {L"Basic colors:", {{L"en",L"Basic colors:"}, {L"es",L"Colores básicos:"}, {L"fr",L"Couleurs de base :"}, {L"de",L"Grundfarben:"}, {L"it",L"Colori di base:"}, {L"ru",L"Основные цвета:"}, {L"zh",L"基本颜色："}, {L"ja",L"基本色："}, {L"ko",L"기본 색상:"}}},
        {L"Custom colors:", {{L"en",L"Custom colors:"}, {L"es",L"Colores personalizados:"}, {L"fr",L"Couleurs personnalisées :"}, {L"de",L"Benutzerdefinierte Farben:"}, {L"it",L"Colori personalizzati:"}, {L"ru",L"Пользовательские цвета:"}, {L"zh",L"自定义颜色："}, {L"ja",L"カスタム色："}, {L"ko",L"사용자 지정 색상:"}}},
        {L"Define Custom Colors >>", {{L"en",L"Define Custom Colors >>"}, {L"es",L"Definir colores personalizados >>"}, {L"fr",L"Définir les couleurs personnalisées >>"}, {L"de",L"Benutzerdefinierte Farben festlegen >>"}, {L"it",L"Definisci colori personalizzati >>"}, {L"ru",L"Задать пользовательские цвета >>"}, {L"zh",L"定义自定义颜色 >>"}, {L"ja",L"カスタム色を定義 >>"}, {L"ko",L"사용자 지정 색상 정의 >>"}}},
        {L"Add to Custom Colors", {{L"en",L"Add to Custom Colors"}, {L"es",L"Añadir a colores personalizados"}, {L"fr",L"Ajouter aux couleurs personnalisées"}, {L"de",L"Zu benutzerdefinierten Farben hinzufügen"}, {L"it",L"Aggiungi ai colori personalizzati"}, {L"ru",L"Добавить в пользовательские цвета"}, {L"zh",L"添加到自定义颜色"}, {L"ja",L"カスタム色に追加"}, {L"ko",L"사용자 지정 색상에 추가"}}},
        {L"Solid", {{L"en",L"Solid"}, {L"es",L"Sólido"}, {L"fr",L"Uni"}, {L"de",L"Vollton"}, {L"it",L"Pieno"}, {L"ru",L"Сплошной"}, {L"zh",L"纯色"}, {L"ja",L"単色"}, {L"ko",L"단색"}}},
        {L"Hue:", {{L"en",L"Hue:"}, {L"es",L"Matiz:"}, {L"fr",L"Teinte :"}, {L"de",L"Farbton:"}, {L"it",L"Tonalità:"}, {L"ru",L"Оттенок:"}, {L"zh",L"色相："}, {L"ja",L"色相："}, {L"ko",L"색조:"}}},
        {L"Sat:", {{L"en",L"Sat:"}, {L"es",L"Sat:"}, {L"fr",L"Sat. :"}, {L"de",L"Sätt.:"}, {L"it",L"Sat.:"}, {L"ru",L"Насыщ.:"}, {L"zh",L"饱和ность："}, {L"ja",L"彩度："}, {L"ko",L"채도:"}}},
        {L"Lum:", {{L"en",L"Lum:"}, {L"es",L"Lum:"}, {L"fr",L"Lum. :"}, {L"de",L"Helligk.:"}, {L"it",L"Lum.:"}, {L"ru",L"Ярк.:"}, {L"zh",L"亮度："}, {L"ja",L"明度："}, {L"ko",L"명도:"}}},
        {L"Red:", {{L"en",L"Red:"}, {L"es",L"Rojo:"}, {L"fr",L"Rouge :"}, {L"de",L"Rot:"}, {L"it",L"Rosso:"}, {L"ru",L"Красный:"}, {L"zh",L"红色："}, {L"ja",L"赤："}, {L"ko",L"빨강:"}}},
        {L"Green:", {{L"en",L"Green:"}, {L"es",L"Verde:"}, {L"fr",L"Vert :"}, {L"de",L"Grün:"}, {L"it",L"Verde:"}, {L"ru",L"Зелёный:"}, {L"zh",L"绿色："}, {L"ja",L"緑："}, {L"ko",L"초록:"}}},
        {L"Blue:", {{L"en",L"Blue:"}, {L"es",L"Azul:"}, {L"fr",L"Bleu :"}, {L"de",L"Blau:"}, {L"it",L"Blu:"}, {L"ru",L"Синий:"}, {L"zh",L"蓝色："}, {L"ja",L"青："}, {L"ko",L"파랑:"}}},
        {L"Cor:", {{L"en",L"Color:"}, {L"es",L"Color:"}, {L"fr",L"Couleur :"}, {L"de",L"Farbe:"}, {L"it",L"Colore:"}, {L"ru",L"Цвет:"}, {L"zh",L"颜色："}, {L"ja",L"色："}, {L"ko",L"색상:"}}},
        {L"Arquivos de áudio (*.mp3;*.wav;*.opus)", {{L"en",L"Audio files (*.mp3;*.wav;*.opus)"}, {L"es",L"Archivos de audio (*.mp3;*.wav;*.opus)"}, {L"fr",L"Fichiers audio (*.mp3;*.wav;*.opus)"}, {L"de",L"Audiodateien (*.mp3;*.wav;*.opus)"}, {L"it",L"File audio (*.mp3;*.wav;*.opus)"}, {L"ru",L"Аудиофайлы (*.mp3;*.wav;*.opus)"}, {L"zh",L"音频文件 (*.mp3;*.wav;*.opus)"}, {L"ja",L"音声ファイル (*.mp3;*.wav;*.opus)"}, {L"ko",L"오디오 파일 (*.mp3;*.wav;*.opus)"}}},
        {L"MP3 (*.mp3)", {{L"en",L"MP3 (*.mp3)"}, {L"es",L"MP3 (*.mp3)"}, {L"fr",L"MP3 (*.mp3)"}, {L"de",L"MP3 (*.mp3)"}, {L"it",L"MP3 (*.mp3)"}, {L"ru",L"MP3 (*.mp3)"}, {L"zh",L"MP3 (*.mp3)"}, {L"ja",L"MP3 (*.mp3)"}, {L"ko",L"MP3 (*.mp3)"}}},
        {L"WAV (*.wav)", {{L"en",L"WAV (*.wav)"}, {L"es",L"WAV (*.wav)"}, {L"fr",L"WAV (*.wav)"}, {L"de",L"WAV (*.wav)"}, {L"it",L"WAV (*.wav)"}, {L"ru",L"WAV (*.wav)"}, {L"zh",L"WAV (*.wav)"}, {L"ja",L"WAV (*.wav)"}, {L"ko",L"WAV (*.wav)"}}},
        {L"Opus (*.opus)", {{L"en",L"Opus (*.opus)"}, {L"es",L"Opus (*.opus)"}, {L"fr",L"Opus (*.opus)"}, {L"de",L"Opus (*.opus)"}, {L"it",L"Opus (*.opus)"}, {L"ru",L"Opus (*.opus)"}, {L"zh",L"Opus (*.opus)"}, {L"ja",L"Opus (*.opus)"}, {L"ko",L"Opus (*.opus)"}}},
        {L"Todos os arquivos (*.*)", {{L"en",L"All files (*.*)"}, {L"es",L"Todos los archivos (*.*)"}, {L"fr",L"Tous les fichiers (*.*)"}, {L"de",L"Alle Dateien (*.*)"}, {L"it",L"Tutti i file (*.*)"}, {L"ru",L"Все файлы (*.*)"}, {L"zh",L"所有文件 (*.*)"}, {L"ja",L"すべてのファイル (*.*)"}, {L"ko",L"모든 파일 (*.*)"}}},
        {L"Texturas BMP/PNG", {{L"en",L"BMP/PNG textures"}, {L"es",L"Texturas BMP/PNG"}, {L"fr",L"Textures BMP/PNG"}, {L"de",L"BMP/PNG-Texturen"}, {L"it",L"Texture BMP/PNG"}, {L"ru",L"Текстуры BMP/PNG"}, {L"zh",L"BMP/PNG 纹理"}, {L"ja",L"BMP/PNG テクスチャ"}, {L"ko",L"BMP/PNG 텍스처"}}},
        {L"Escolher textura para este pincel", {{L"en",L"Choose texture for this brush"}, {L"es",L"Elegir textura para este pincel"}, {L"fr",L"Choisir une texture pour ce pinceau"}, {L"de",L"Textur für diesen Pinsel auswählen"}, {L"it",L"Scegli texture per questo pennello"}, {L"ru",L"Выбрать текстуру для этой кисти"}, {L"zh",L"为此笔刷选择纹理"}, {L"ja",L"このブラシのテクスチャを選択"}, {L"ko",L"이 브러시의 텍스처 선택"}}},
        {L"Importar pincel ou recursos de outro programa", {{L"en",L"Import brush or resources from another program"}, {L"es",L"Importar pincel o recursos de otro programa"}, {L"fr",L"Importer un pinceau ou des ressources d’un autre programme"}, {L"de",L"Pinsel oder Ressourcen aus einem anderen Programm importieren"}, {L"it",L"Importa pennello o risorse da un altro programma"}, {L"ru",L"Импортировать кисть или ресурсы из другой программы"}, {L"zh",L"从其他程序导入笔刷或资源"}, {L"ja",L"他のプログラムからブラシやリソースを読み込む"}, {L"ko",L"다른 프로그램의 브러시 또는 리소스 가져오기"}}},
        {L"Todos os arquivos", {{L"en",L"All files"}, {L"es",L"Todos los archivos"}, {L"fr",L"Tous les fichiers"}, {L"de",L"Alle Dateien"}, {L"it",L"Tutti i file"}, {L"ru",L"Все файлы"}, {L"zh",L"所有文件"}, {L"ja",L"すべてのファイル"}, {L"ko",L"모든 파일"}}},
        {L"Preferências do Kodanima", {{L"en",L"Kodanima Preferences"}, {L"es",L"Preferencias de Kodanima"}, {L"fr",L"Préférences Kodanima"}, {L"de",L"Kodanima-Einstellungen"}, {L"it",L"Preferenze Kodanima"}, {L"ru",L"Настройки Kodanima"}, {L"zh",L"Kodanima 偏好设置"}, {L"ja",L"Kodanima の設定"}, {L"ko",L"Kodanima 환경설정"}}},
        {L"Anti-overflow — limitar ao recorte válido", {{L"en",L"Anti-overflow — limit to valid drawing area"}, {L"es",L"Antidesbordamiento — limitar al área válida"}, {L"fr",L"Anti-débordement — limiter à la zone valide"}, {L"de",L"Überlaufschutz — auf gültigen Zeichenbereich begrenzen"}, {L"it",L"Anti-overflow — limita all’area valida"}, {L"ru",L"Защита от выхода — ограничить областью рисования"}, {L"zh",L"防溢出 — 限制在有效绘图区域"}, {L"ja",L"オーバーフロー防止 — 有効な描画領域に制限"}, {L"ko",L"오버플로 방지 — 유효한 그리기 영역으로 제한"}}},
        {L"Ativar Starting / Ending", {{L"en",L"Enable Starting / Ending"}, {L"es",L"Activar Inicio / Final"}, {L"fr",L"Activer Début / Fin"}, {L"de",L"Anfang / Ende aktivieren"}, {L"it",L"Attiva Inizio / Fine"}, {L"ru",L"Включить начало / конец"}, {L"zh",L"启用起始 / 结束"}, {L"ja",L"開始 / 終了を有効化"}, {L"ko",L"시작 / 끝 사용"}}},
        {L"Tamanho máximo (pressão alta)", {{L"en",L"Maximum size (high pressure)"}, {L"es",L"Tamaño máximo (presión alta)"}, {L"fr",L"Taille maximale (pression élevée)"}, {L"de",L"Maximalgröße (hoher Druck)"}, {L"it",L"Dimensione massima (pressione alta)"}, {L"ru",L"Максимальный размер (высокое давление)"}, {L"zh",L"最大尺寸（高压力）"}, {L"ja",L"最大サイズ（高筆圧）"}, {L"ko",L"최대 크기(높은 압력)"}}},
        {L"Tamanho mínimo (pressão baixa)", {{L"en",L"Minimum size (low pressure)"}, {L"es",L"Tamaño mínimo (presión baja)"}, {L"fr",L"Taille minimale (pression faible)"}, {L"de",L"Mindestgröße (niedriger Druck)"}, {L"it",L"Dimensione minima (pressione bassa)"}, {L"ru",L"Минимальный размер (низкое давление)"}, {L"zh",L"最小尺寸（低压力）"}, {L"ja",L"最小サイズ（低筆圧）"}, {L"ko",L"최소 크기(낮은 압력)"}}},
        {L"Duração do quadro em segundos:", {{L"en",L"Frame duration in seconds:"}, {L"es",L"Duración del fotograma en segundos:"}, {L"fr",L"Durée de l’image en secondes :"}, {L"de",L"Bilddauer in Sekunden:"}, {L"it",L"Durata del fotogramma in secondi:"}, {L"ru",L"Длительность кадра в секундах:"}, {L"zh",L"帧持续时间（秒）："}, {L"ja",L"フレームの長さ（秒）："}, {L"ko",L"프레임 지속 시간(초):"}}},
        {L"Opacidade Onion Skin (0-100%):", {{L"en",L"Onion Skin opacity (0–100%):"}, {L"es",L"Opacidad de papel cebolla (0–100%):"}, {L"fr",L"Opacité de pelure d’oignon (0–100 %) :"}, {L"de",L"Zwiebelschalen-Deckkraft (0–100 %):"}, {L"it",L"Opacità Onion Skin (0–100%):"}, {L"ru",L"Непрозрачность onion skin (0–100%):"}, {L"zh",L"洋葱皮不透明度（0–100%）："}, {L"ja",L"オニオンスキンの不透明度（0–100%）："}, {L"ko",L"어니언 스킨 불투명도(0–100%):"}}},
        {L"Conferindo e finalizando os dados gravados...", {{L"en",L"Checking and finalizing written data..."}, {L"es",L"Comprobando y finalizando los datos escritos..."}, {L"fr",L"Vérification et finalisation des données écrites..."}, {L"de",L"Geschriebene Daten werden geprüft und abgeschlossen..."}, {L"it",L"Verifica e finalizzazione dei dati scritti..."}, {L"ru",L"Проверка и завершение записи данных..."}, {L"zh",L"正在检查并完成写入的数据..."}, {L"ja",L"書き込まれたデータを確認して完了しています..."}, {L"ko",L"기록된 데이터 확인 및 마무리 중..."}}},
        {L"O arquivo foi gerado com sucesso.", {{L"en",L"The file was created successfully."}, {L"es",L"El archivo se creó correctamente."}, {L"fr",L"Le fichier a été créé avec succès."}, {L"de",L"Die Datei wurde erfolgreich erstellt."}, {L"it",L"Il file è stato creato correttamente."}, {L"ru",L"Файл успешно создан."}, {L"zh",L"文件已成功生成。"}, {L"ja",L"ファイルを正常に作成しました。"}, {L"ko",L"파일이 성공적으로 생성되었습니다."}}},
        {L"A renderização falhou. Confira a mensagem de erro.", {{L"en",L"Rendering failed. Check the error message."}, {L"es",L"La renderización falló. Revise el mensaje de error."}, {L"fr",L"Le rendu a échoué. Consultez le message d’erreur."}, {L"de",L"Das Rendern ist fehlgeschlagen. Prüfen Sie die Fehlermeldung."}, {L"it",L"Rendering non riuscito. Controlla il messaggio di errore."}, {L"ru",L"Ошибка рендеринга. Проверьте сообщение об ошибке."}, {L"zh",L"渲染失败。请查看错误消息。"}, {L"ja",L"レンダリングに失敗しました。エラーメッセージを確認してください。"}, {L"ko",L"렌더링에 실패했습니다. 오류 메시지를 확인하세요."}}},
        {L"As mudanças aplicadas ficam ativas durante a edição.", {{L"en",L"Applied changes remain active while editing."}, {L"es",L"Los cambios aplicados permanecen activos durante la edición."}, {L"fr",L"Les modifications appliquées restent actives pendant l’édition."}, {L"de",L"Angewendete Änderungen bleiben während der Bearbeitung aktiv."}, {L"it",L"Le modifiche applicate restano attive durante la modifica."}, {L"ru",L"Применённые изменения действуют во время редактирования."}, {L"zh",L"应用的更改在编辑期间保持生效。"}, {L"ja",L"適用した変更は編集中に有効です。"}, {L"ko",L"적용된 변경 사항은 편집 중에 유지됩니다."}}},
        {L"Renomear pincel", {{L"en",L"Rename brush"}, {L"es",L"Renombrar pincel"}, {L"fr",L"Renommer le pinceau"}, {L"de",L"Pinsel umbenennen"}, {L"it",L"Rinomina pennello"}, {L"ru",L"Переименовать кисть"}, {L"zh",L"重命名笔刷"}, {L"ja",L"ブラシ名を変更"}, {L"ko",L"브러시 이름 변경"}}},
        {L"Pincéis do Kodanima — erro ao montar lista", {{L"en",L"Kodanima brushes — error building list"}, {L"es",L"Pinceles de Kodanima — error al crear la lista"}, {L"fr",L"Pinceaux Kodanima — erreur lors de la création de la liste"}, {L"de",L"Kodanima-Pinsel — Fehler beim Erstellen der Liste"}, {L"it",L"Pennelli Kodanima — errore durante la creazione dell’elenco"}, {L"ru",L"Кисти Kodanima — ошибка создания списка"}, {L"zh",L"Kodanima 笔刷 — 创建列表时出错"}, {L"ja",L"Kodanima ブラシ — リスト作成エラー"}, {L"ko",L"Kodanima 브러시 — 목록 생성 오류"}}},
        {L"Escolha a pasta onde salvar a animação", {{L"en",L"Choose the folder where the animation will be saved"}, {L"es",L"Elija la carpeta donde guardar la animación"}, {L"fr",L"Choisissez le dossier où enregistrer l’animation"}, {L"de",L"Wählen Sie den Ordner zum Speichern der Animation"}, {L"it",L"Scegli la cartella in cui salvare l’animazione"}, {L"ru",L"Выберите папку для сохранения анимации"}, {L"zh",L"选择保存动画的文件夹"}, {L"ja",L"アニメーションを保存するフォルダーを選択"}, {L"ko",L"애니메이션을 저장할 폴더 선택"}}},
    };
    return t;
}

static std::wstring KodaTraduzirTexto(const wchar_t* original) {
    std::wstring src=original?original:L"";
    if(src.empty()) return src;
    const std::wstring lang=KodaIdiomaBase();
    if(lang==L"pt") return src;
    const auto& catalog=KodaCatalogoTraducao();
    auto row=catalog.find(src);
    if(row!=catalog.end()) {
        auto translated=row->second.find(lang);
        if(translated!=row->second.end()) return translated->second;
    }
    const auto& extra=KodaCatalogoExtraTraducao();
    auto e=extra.find(src);
    if(e!=extra.end()) {
        auto translated=e->second.find(lang);
        if(translated!=e->second.end()) return translated->second;
    }
    const auto& supplement=KodaCatalogoUiSupplement();
    auto u=supplement.find(src);
    if(u!=supplement.end()) {
        auto translated=u->second.find(lang);
        if(translated!=u->second.end()) return translated->second;
    }
    return src;
}

static std::wstring KodaNormalizarQuebrasAjuda(const wchar_t* text) {
    std::wstring in=text?text:L"", out;
    out.reserve(in.size()+32);
    for(size_t i=0;i<in.size();++i) {
        if(in[i]==L'\n' && (i==0 || in[i-1]!=L'\r')) out+=L'\r';
        out+=in[i];
    }
    return out;
}
static std::wstring KodaMontarTextoExplicacao() {
    static const std::map<std::wstring,std::wstring> docs = {
        {L"pt", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — EXPLICAÇÃO

1. CRIAR, ABRIR E SALVAR PROJETOS
Na tela inicial, escolha Criar para definir nome, pasta, largura, altura e FPS. Escolha Abrir para continuar um projeto existente. Durante a edição, use Ctrl+S para salvar. Mantenha a pasta do projeto e a subpasta assets juntas para não perder mídias importadas.

2. CANVAS, DESENHO E NAVEGAÇÃO
O canvas é a área de desenho. Escolha uma ferramenta na barra lateral e desenhe com mouse ou caneta. Use Espaço enquanto arrasta para mover a visualização, sem deslocar o desenho. A posição do cursor e a janela sob ele influenciam quais atalhos serão acionados.

3. LINHA DO TEMPO E QUADROS
A timeline organiza os quadros por ordem e duração. Use + Quadro para criar, Inserir para inserir um quadro e Excluir para remover o quadro selecionado. Clique em um quadro para selecioná-lo e visualizar seu desenho no canvas. A régua superior representa o tempo em segundos.

4. MARCADOR VERDE: AJUSTAR A DURAÇÃO
No marcador verde da borda esquerda do quadro, mantenha o botão direito do mouse pressionado. O cursor salta para o fim atual desse quadro; continue arrastando para definir a nova duração e solte para confirmar. Com a mesa digitalizadora, toque com a ponta e mantenha pressionado — não precisa de botão direito físico. Também é possível digitar a duração no campo Dur. e pressionar Enter.

5. REORDENAR QUADROS
Mantenha o botão direito pressionado sobre o número do quadro e arraste-o para a posição desejada. Com a caneta, mantenha a ponta pressionada sobre o número e arraste. Ao soltar, os quadros são reorganizados e renumerados. Os atalhos Num 4 e Num 6 também movem o quadro selecionado uma posição para a esquerda ou para a direita quando o cursor está sobre a timeline.

6. PINÇA DE TEMPO E ÁUDIO
Arraste a pinça na régua superior para percorrer o tempo. Se a reprodução estiver ativa, começar a arrastar a pinça interrompe o Play. Marque Ouvir ao mover a pinça para ouvir o áudio durante a busca. Use Play para iniciar/pausar a prévia e Loop para repetir o intervalo ativado.

7. FERRAMENTAS E ATALHOS
Lápis desenha; Borracha apaga; Balde preenche; Laço cria uma seleção; Roda de cores escolhe a cor; Conta-gotas captura uma cor do canvas; Renderizar abre as opções de exportação. Atalhos padrão: F1 preferências; F2/F6 novo quadro; F3 Onion Skin; F4 renderizar; F5 lápis; F7 inserir quadro; F8 borracha; F9 balde; F10 roda de cores; F12 conta-gotas; vírgula/quadro anterior; ponto/próximo quadro; Tab maximizar/restaurar canvas; Enter reproduzir/pausar quando o cursor está na timeline; Delete excluir conforme a área do cursor. Configure as teclas na categoria Atalhos.

8. COMO FUNCIONA O BOTÃO DIREITO
A ação depende da área sob o cursor: no canvas, o comando é contextual à ferramenta/seleção; na timeline, o botão direito pressionado arrasta o marcador verde para ajustar duração ou o número para reordenar. Com a caneta, o toque prolongado simula esses gestos na timeline. Se um atalho não agir, confira se o cursor está no canvas ou na timeline e se não está editando um campo de texto.

9. CAMADAS E ONION SKIN
Selecione uma camada na coluna à esquerda. O ícone de visibilidade mostra/oculta a camada. Use Onion para ver quadros vizinhos como referência; ajuste a opacidade pelo campo %. Criar, remover ou mover camadas altera a organização do desenho, não a resolução da tela.

10. RESOLUÇÃO, DPI E QUALIDADE
Largura e altura definem quantos pixels o desenho contém: mais pixels permitem mais detalhes, mas usam mais memória e podem deixar a edição/renderização mais lenta. DPI descreve a densidade para dimensionamento físico/impressão; aumentar o DPI sozinho não cria detalhes de pixels. Defina a resolução adequada antes de desenhar.

11. RENDERIZAR / EXPORTAR
Clique em Renderizar na barra de ferramentas ou pressione F4. Escolha o formato, nome e pasta de saída. O programa exporta MP4/H.264, GIF animado ou MOV com vídeo PNG; GIF/MOV podem usar transparência, e MP4/MOV podem incluir áudio. MP4/H.264 precisa de Windows 7 ou superior e de suporte ao Media Foundation/codificador H.264; no Windows XP use GIF ou MOV/PNG.

12. IDIOMA E CONFIGURAÇÕES
Na tela inicial, use Language para escolher o idioma. Em Preferências, ajuste tema, tamanho da interface/texto, motor gráfico, mesa digitalizadora, altura das linhas da timeline, histórico e atalhos. Algumas opções só entram em vigor após salvar as configurações.

ATALHOS NOVOS DA TIMELINE
Num + aumenta a duração do quadro selecionado em 0,1 segundo; Num - diminui em 0,1 segundo, respeitando o mínimo. Num 4 move o quadro selecionado para a esquerda; Num 6 move para a direita. Essas quatro ações só funcionam quando o cursor está sobre a timeline. Os gestos do marcador verde e de reordenação continuam disponíveis com botão direito do mouse ou toque prolongado da caneta.)KODA")},
        {L"en", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — EXPLANATION

1. CREATE, OPEN AND SAVE PROJECTS
On the start screen, choose Create to set the name, folder, width, height and FPS. Choose Open to continue an existing project. Press Ctrl+S while editing to save. Keep the project folder and its assets subfolder together so imported media remains available.

2. CANVAS, DRAWING AND NAVIGATION
The canvas is the drawing area. Choose a tool on the side toolbar and draw with the mouse or pen. Hold Space while dragging to pan the view without moving the artwork. The cursor position and the window beneath it affect which shortcuts run.

3. TIMELINE AND FRAMES
The timeline organizes frames by order and duration. Use + Frame to create one, Insert to insert a frame, and Delete to remove the selected frame. Click a frame to select it and show its drawing on the canvas. The top ruler represents time in seconds.

4. GREEN MARKER: CHANGE DURATION
On the green marker at the frame’s left edge, hold the mouse right button. The cursor jumps to the frame’s current end; keep dragging to set the new duration, then release to confirm. With a drawing tablet, touch with the pen tip and keep it pressed—no physical right button is needed. You can also type a duration in the Dur. field and press Enter.

5. REORDER FRAMES
Hold the mouse right button on a frame number and drag it to the desired position. With a pen, keep the tip pressed on the number and drag. On release, frames are reordered and renumbered. Num 4 and Num 6 also move the selected frame one position left or right when the cursor is over the timeline.

6. PLAYHEAD AND AUDIO
Drag the playhead on the top ruler to scrub through time. If playback is active, starting to drag the playhead stops Play. Enable Scrub audio with playhead to hear audio while seeking. Use Play to start/pause the preview and Loop to repeat the enabled range.

7. TOOLS AND SHORTCUTS
Pencil draws; Eraser removes; Bucket fills; Lasso makes a selection; Color wheel chooses a color; Eyedropper picks a canvas color; Render opens export options. Defaults: F1 preferences; F2/F6 new frame; F3 Onion Skin; F4 render; F5 pencil; F7 insert frame; F8 eraser; F9 bucket; F10 color wheel; F12 eyedropper; comma/previous frame; period/next frame; Tab maximize/restore canvas; Enter play/pause when the cursor is on the timeline; Delete removes an item according to cursor area. Customize keys in Shortcuts.

8. HOW RIGHT-CLICK WORKS
The action depends on what is beneath the cursor: on the canvas, the command depends on the tool/selection; on the timeline, holding right-click drags the green marker to change duration or a frame number to reorder it. On a tablet, a sustained pen-tip touch simulates these timeline gestures. If a shortcut does not act, check whether the cursor is over the canvas or timeline and whether a text field is being edited.

9. LAYERS AND ONION SKIN
Select a layer in the left column. Its visibility icon shows or hides it. Use Onion to see neighboring frames as references; adjust opacity with the % field. Adding, removing or moving layers changes drawing organization, not canvas resolution.

10. RESOLUTION, DPI AND QUALITY
Width and height determine the number of pixels in the drawing: more pixels allow more detail but use more memory and may slow editing/rendering. DPI describes density for physical sizing/printing; increasing DPI alone does not add pixel detail. Choose a suitable resolution before drawing.

11. RENDER / EXPORT
Click Render on the toolbar or press F4. Choose the format, name and output folder. The program exports MP4/H.264, animated GIF or MOV with PNG video; GIF/MOV can use transparency, and MP4/MOV can include audio. MP4/H.264 requires Windows 7 or later and Media Foundation/H.264 encoder support; on Windows XP, use GIF or MOV/PNG.

12. LANGUAGE AND SETTINGS
On the start screen, use Language to choose the interface language. In Preferences, adjust the theme, interface/text size, graphics engine, tablet, timeline row height, history and shortcuts. Some options take effect after you save settings.

NEW TIMELINE SHORTCUTS
Num + increases the selected frame duration by 0.1 seconds; Num - decreases it, respecting the minimum. Num 4 moves the selected frame left; Num 6 moves it right. These four actions work only when the cursor is over the timeline. The green-marker and frame-reorder gestures remain available with right-click drag or a sustained pen-tip touch.)KODA")},
        {L"es", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — EXPLICACIÓN

1. CREAR, ABRIR Y GUARDAR PROYECTOS
En la pantalla inicial, elige Crear para definir nombre, carpeta, ancho, alto y FPS. Elige Abrir para continuar un proyecto existente. Pulsa Ctrl+S durante la edición para guardar. Conserva juntos la carpeta del proyecto y su subcarpeta assets para mantener los archivos importados.

2. LIENZO, DIBUJO Y NAVEGACIÓN
El lienzo es el área de dibujo. Elige una herramienta en la barra lateral y dibuja con el ratón o el lápiz. Mantén Espacio mientras arrastras para desplazar la vista sin mover el dibujo. La posición del cursor y la ventana situada debajo determinan qué atajos se ejecutan.

3. LÍNEA DE TIEMPO Y FOTOGRAMAS
La línea de tiempo organiza los fotogramas por orden y duración. Usa + Fotograma para crear uno, Insertar para insertar y Eliminar para borrar el seleccionado. Haz clic en un fotograma para seleccionarlo y mostrar su dibujo. La regla superior representa el tiempo en segundos.

4. MARCADOR VERDE: CAMBIAR LA DURACIÓN
En el marcador verde del borde izquierdo del fotograma, mantén pulsado el botón derecho del ratón. El cursor salta al final actual del fotograma; sigue arrastrando para definir la nueva duración y suelta para confirmar. Con una tableta, toca con la punta del lápiz y mantenla presionada; no hace falta un botón derecho físico. También puedes escribir la duración en Dur. y pulsar Enter.

5. REORDENAR FOTOGRAMAS
Mantén el botón derecho sobre el número del fotograma y arrástralo a la posición deseada. Con el lápiz, mantén la punta sobre el número y arrastra. Al soltar, los fotogramas se reordenan y renumeran. Num 4 y Num 6 también mueven el fotograma seleccionado una posición a la izquierda o derecha cuando el cursor está sobre la línea de tiempo.

6. CABEZAL DE REPRODUCCIÓN Y AUDIO
Arrastra el cabezal por la regla superior para recorrer el tiempo. Si la reproducción está activa, empezar a arrastrarlo detiene Play. Activa Escuchar al mover el cabezal para oír el audio mientras buscas. Usa Play para iniciar/pausar y Loop para repetir el intervalo activado.

7. HERRAMIENTAS Y ATAJOS
Lápiz dibuja; Borrador elimina; Cubo rellena; Lazo selecciona; Rueda de color elige color; Cuentagotas toma un color del lienzo; Renderizar abre la exportación. Predeterminados: F1 preferencias; F2/F6 nuevo fotograma; F3 Onion Skin; F4 renderizar; F5 lápiz; F7 insertar; F8 borrador; F9 cubo; F10 rueda de color; F12 cuentagotas; coma/anterior; punto/siguiente; Tab maximizar/restaurar lienzo; Enter reproducir/pausar con el cursor en la línea de tiempo; Delete borrar según el área. Cambia las teclas en Atajos.

8. CÓMO FUNCIONA EL BOTÓN DERECHO
La acción depende de dónde esté el cursor: en el lienzo, depende de la herramienta o selección; en la línea de tiempo, mantener el botón derecho arrastra el marcador verde para cambiar la duración o el número para reordenar. En una tableta, mantener la punta presionada simula esos gestos. Si un atajo no funciona, comprueba dónde está el cursor y que no estés editando un campo de texto.

9. CAPAS Y ONION SKIN
Selecciona una capa en la columna izquierda. Su icono de visibilidad la muestra u oculta. Usa Onion para ver fotogramas vecinos como referencia y ajusta la opacidad con el campo %. Añadir, eliminar o mover capas cambia la organización, no la resolución del lienzo.

10. RESOLUCIÓN, DPI Y CALIDAD
El ancho y el alto determinan los píxeles del dibujo: más píxeles permiten más detalle, pero usan más memoria y pueden ralentizar la edición/renderización. DPI describe la densidad para el tamaño físico o impresión; aumentar DPI por sí solo no añade detalle de píxel. Elige la resolución antes de dibujar.

11. RENDERIZAR / EXPORTAR
Pulsa Renderizar en la barra o F4. Elige formato, nombre y carpeta. Se exporta MP4/H.264, GIF animado o MOV con vídeo PNG; GIF/MOV admiten transparencia y MP4/MOV pueden incluir audio. MP4/H.264 requiere Windows 7 o posterior y compatibilidad con Media Foundation/codificador H.264; en Windows XP usa GIF o MOV/PNG.

12. IDIOMA Y CONFIGURACIÓN
En la pantalla inicial, usa Language para elegir el idioma. En Preferencias, ajusta tema, tamaño de interfaz/texto, motor gráfico, tableta, altura de filas, historial y atajos. Algunas opciones se aplican después de guardar.

NUEVOS ATAJOS DE LA LÍNEA DE TIEMPO
Num + aumenta la duración del fotograma seleccionado 0,1 s; Num - la reduce respetando el mínimo. Num 4 mueve el fotograma a la izquierda y Num 6 a la derecha. Solo funcionan cuando el cursor está sobre la línea de tiempo. Los gestos del marcador verde y de reordenación siguen disponibles con botón derecho o manteniendo la punta del lápiz.)KODA")},
        {L"fr", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — EXPLICATION

1. CRÉER, OUVRIR ET ENREGISTRER UN PROJET
Sur l’écran d’accueil, choisissez Créer pour définir le nom, le dossier, la largeur, la hauteur et les FPS. Choisissez Ouvrir pour reprendre un projet. Appuyez sur Ctrl+S pendant l’édition pour enregistrer. Gardez le dossier du projet et son sous-dossier assets ensemble.

2. CANEVAS, DESSIN ET NAVIGATION
Le canevas est la zone de dessin. Choisissez un outil dans la barre latérale et dessinez avec la souris ou le stylet. Maintenez Espace pendant le glissement pour déplacer la vue sans déplacer le dessin. La position du curseur et la fenêtre sous celui-ci déterminent les raccourcis actifs.

3. CHRONOLOGIE ET IMAGES
La chronologie organise les images selon leur ordre et leur durée. Utilisez + Image pour en créer une, Insérer pour en insérer et Supprimer pour retirer l’image sélectionnée. Cliquez sur une image pour la sélectionner et afficher son dessin. La règle supérieure indique le temps en secondes.

4. MARQUEUR VERT : MODIFIER LA DURÉE
Sur le marqueur vert au bord gauche de l’image, maintenez le bouton droit de la souris. Le curseur saute à la fin actuelle de l’image ; continuez à le faire glisser pour modifier la durée, puis relâchez. Avec une tablette, touchez avec la pointe du stylet et maintenez-la appuyée : aucun bouton droit physique n’est nécessaire. Vous pouvez aussi saisir la durée dans Dur. et appuyer sur Entrée.

5. RÉORGANISER LES IMAGES
Maintenez le bouton droit sur le numéro d’une image et faites-le glisser à l’endroit voulu. Avec le stylet, maintenez la pointe sur le numéro puis faites glisser. À la libération, les images sont réorganisées et renumérotées. Num 4 et Num 6 déplacent aussi l’image sélectionnée d’une position vers la gauche ou la droite lorsque le curseur est sur la chronologie.

6. TÊTE DE LECTURE ET AUDIO
Faites glisser la tête de lecture sur la règle pour parcourir le temps. Si la lecture est active, commencer à déplacer la tête arrête Play. Activez Écouter pendant le déplacement de la tête pour entendre l’audio pendant la recherche. Play lance/met en pause l’aperçu et Loop répète la plage activée.

7. OUTILS ET RACCOURCIS
Crayon dessine ; Gomme efface ; Pot remplit ; Lasso sélectionne ; Roue de couleurs choisit la couleur ; Pipette prélève une couleur ; Rendu ouvre l’exportation. Par défaut : F1 préférences ; F2/F6 nouvelle image ; F3 Onion Skin ; F4 rendu ; F5 crayon ; F7 insérer ; F8 gomme ; F9 pot ; F10 roue ; F12 pipette ; virgule/précédente ; point/suivante ; Tab maximiser/restaurer le canevas ; Entrée lecture/pause si le curseur est sur la chronologie ; Delete supprimer selon la zone. Modifiez les touches dans Raccourcis.

8. FONCTION DU BOUTON DROIT
L’action dépend de la zone sous le curseur : sur le canevas, elle dépend de l’outil ou de la sélection ; sur la chronologie, maintenir le bouton droit permet de déplacer le marqueur vert pour changer la durée ou le numéro pour réorganiser. Sur tablette, un appui prolongé du stylet simule ces gestes. Si un raccourci ne fonctionne pas, vérifiez la position du curseur et qu’aucun champ texte n’est en cours d’édition.

9. CALQUES ET ONION SKIN
Sélectionnez un calque dans la colonne de gauche. Son icône de visibilité l’affiche ou le masque. Onion affiche les images voisines comme référence ; réglez l’opacité avec le champ %. Ajouter, supprimer ou déplacer des calques change l’organisation, pas la résolution du canevas.

10. RÉSOLUTION, DPI ET QUALITÉ
La largeur et la hauteur déterminent le nombre de pixels : plus de pixels donnent plus de détails, mais utilisent davantage de mémoire et peuvent ralentir l’édition/le rendu. Le DPI décrit la densité pour la taille physique ou l’impression ; augmenter le DPI seul n’ajoute pas de détails en pixels. Choisissez la résolution avant de dessiner.

11. RENDU / EXPORTATION
Cliquez sur Rendu dans la barre ou appuyez sur F4. Choisissez le format, le nom et le dossier de sortie. L’export prend en charge MP4/H.264, GIF animé ou MOV avec vidéo PNG ; GIF/MOV peuvent utiliser la transparence et MP4/MOV peuvent inclure l’audio. MP4/H.264 nécessite Windows 7+ et Media Foundation/codeur H.264 ; sous Windows XP, utilisez GIF ou MOV/PNG.

12. LANGUE ET PARAMÈTRES
Sur l’écran d’accueil, utilisez Language pour choisir la langue. Dans Préférences, réglez le thème, la taille de l’interface/du texte, le moteur graphique, la tablette, les lignes de chronologie, l’historique et les raccourcis. Certaines options prennent effet après l’enregistrement.

NOUVEAUX RACCOURCIS DE CHRONOLOGIE
Num + augmente la durée de 0,1 s ; Num - la réduit sans dépasser le minimum. Num 4 déplace l’image sélectionnée à gauche et Num 6 à droite. Ces actions fonctionnent lorsque le curseur se trouve sur la chronologie. Les gestes du marqueur vert et de réorganisation restent disponibles avec le bouton droit ou un appui prolongé du stylet.)KODA")},
        {L"de", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — ERKLÄRUNG

1. PROJEKTE ERSTELLEN, ÖFFNEN UND SPEICHERN
Wählen Sie auf dem Startbildschirm Erstellen, um Name, Ordner, Breite, Höhe und FPS festzulegen. Mit Öffnen laden Sie ein vorhandenes Projekt. Drücken Sie während der Bearbeitung Strg+S zum Speichern. Bewahren Sie den Projektordner und den Unterordner assets zusammen auf.

2. LEINWAND, ZEICHNEN UND NAVIGATION
Die Leinwand ist der Zeichenbereich. Wählen Sie ein Werkzeug in der Seitenleiste und zeichnen Sie mit Maus oder Stift. Halten Sie beim Ziehen die Leertaste gedrückt, um die Ansicht zu verschieben, ohne die Zeichnung zu bewegen. Cursorposition und Fenster darunter bestimmen, welche Tastenkürzel wirken.

3. ZEITLEISTE UND BILDER
Die Zeitleiste ordnet Bilder nach Reihenfolge und Dauer. Mit + Bild erstellen Sie eines, mit Einfügen fügen Sie eines ein, und mit Löschen entfernen Sie das ausgewählte Bild. Klicken Sie auf ein Bild, um es auszuwählen und auf der Leinwand anzuzeigen. Das obere Lineal zeigt Sekunden.

4. GRÜNER MARKER: DAUER ÄNDERN
Halten Sie auf dem grünen Marker am linken Bildrand die rechte Maustaste gedrückt. Der Cursor springt zum aktuellen Ende des Bildes; ziehen Sie weiter, um die Dauer zu ändern, und lassen Sie los. Beim Grafiktablet reicht es, die Stiftspitze aufzusetzen und gedrückt zu halten; eine physische rechte Taste ist nicht nötig. Alternativ Dauer im Feld Dur. eingeben und Enter drücken.

5. BILDER NEU ANORDNEN
Halten Sie die rechte Maustaste auf einer Bildnummer gedrückt und ziehen Sie sie an die gewünschte Position. Mit dem Stift die Spitze auf der Nummer halten und ziehen. Beim Loslassen werden die Bilder neu angeordnet und nummeriert. Num 4 und Num 6 verschieben das ausgewählte Bild ebenfalls um eine Position nach links oder rechts, wenn der Cursor über der Zeitleiste steht.

6. ABSpielKOPF UND AUDIO
Ziehen Sie den Abspielkopf auf dem oberen Lineal, um durch die Zeit zu suchen. Wenn die Wiedergabe läuft, stoppt das Starten des Ziehens Play. Aktivieren Sie Audio beim Bewegen des Abspielkopfs hören, um während der Suche Ton zu hören. Play startet/pausiert die Vorschau, Loop wiederholt den aktivierten Bereich.

7. WERKZEUGE UND TASTENKÜRZEL
Stift zeichnet; Radierer löscht; Füllwerkzeug füllt; Lasso wählt aus; Farbrad wählt eine Farbe; Pipette nimmt eine Canvas-Farbe auf; Rendern öffnet den Export. Standard: F1 Einstellungen; F2/F6 neues Bild; F3 Onion Skin; F4 Rendern; F5 Stift; F7 Einfügen; F8 Radierer; F9 Füllwerkzeug; F10 Farbrad; F12 Pipette; Komma/vorheriges Bild; Punkt/nächstes Bild; Tab Leinwand maximieren/wiederherstellen; Enter Wiedergabe/Pause bei Cursor über der Zeitleiste; Delete löscht je nach Cursorbereich. Tasten lassen sich unter Tastenkürzel ändern.

8. FUNKTION DER RECHTEN MAUSTASTE
Die Aktion hängt vom Bereich unter dem Cursor ab: auf der Leinwand vom Werkzeug/der Auswahl; auf der Zeitleiste zieht ein gehaltener Rechtsklick den grünen Marker zur Daueränderung oder eine Bildnummer zum Neuordnen. Am Tablet simuliert längeres Halten der Stiftspitze diese Gesten. Falls ein Kürzel nicht wirkt, prüfen Sie die Cursorposition und ob ein Textfeld bearbeitet wird.

9. EBENEN UND ONION SKIN
Wählen Sie links eine Ebene. Das Sichtbarkeitssymbol blendet sie ein oder aus. Onion zeigt Nachbarbilder als Referenz; die Deckkraft stellen Sie über das %-Feld ein. Ebenen verändern die Organisation, nicht die Leinwandauflösung.

10. AUFLÖSUNG, DPI UND QUALITÄT
Breite und Höhe bestimmen die Pixelzahl: mehr Pixel ermöglichen mehr Details, benötigen aber mehr Speicher und können Bearbeitung/Rendering verlangsamen. DPI beschreibt die Dichte für physische Größe/Druck; mehr DPI allein erzeugt keine zusätzlichen Pixel-Details. Wählen Sie die Auflösung vor dem Zeichnen.

11. RENDERN / EXPORTIEREN
Klicken Sie auf Rendern in der Werkzeugleiste oder drücken Sie F4. Wählen Sie Format, Dateiname und Zielordner. Unterstützt werden MP4/H.264, animiertes GIF oder MOV mit PNG-Video; GIF/MOV können Transparenz und MP4/MOV Audio enthalten. MP4/H.264 benötigt Windows 7+ und Media Foundation/H.264-Encoder-Unterstützung; unter Windows XP GIF oder MOV/PNG nutzen.

12. SPRACHE UND EINSTELLUNGEN
Auf dem Startbildschirm wählen Sie Language. Unter Einstellungen können Sie Design, Oberflächen-/Textgröße, Grafikengine, Tablet, Zeitleistenzeilen, Verlauf und Tastenkürzel ändern. Einige Optionen werden nach dem Speichern wirksam.

NEUE ZEITLEISTEN-KÜRZEL
Num + erhöht die Bilddauer um 0,1 Sekunden; Num - verringert sie bis zum Mindestwert. Num 4 verschiebt das ausgewählte Bild nach links, Num 6 nach rechts. Diese Aktionen funktionieren nur bei Cursor über der Zeitleiste. Marker- und Neuordnungs-Gesten funktionieren weiterhin mit Rechtsklick-Ziehen oder längerem Stiftdruck.)KODA")},
        {L"it", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — SPIEGAZIONE

1. CREARE, APRIRE E SALVARE I PROGETTI
Nella schermata iniziale scegli Crea per impostare nome, cartella, larghezza, altezza e FPS. Scegli Apri per continuare un progetto esistente. Premi Ctrl+S durante la modifica per salvare. Mantieni insieme la cartella del progetto e la sottocartella assets.

2. CANVAS, DISEGNO E NAVIGAZIONE
Il canvas è l’area di disegno. Scegli uno strumento dalla barra laterale e disegna con mouse o penna. Tieni premuto Spazio mentre trascini per spostare la vista senza muovere il disegno. La posizione del cursore e la finestra sottostante determinano quali scorciatoie vengono eseguite.

3. TIMELINE E FOTOGRAMMI
La timeline organizza i fotogrammi per ordine e durata. Usa + Fotogramma per crearne uno, Inserisci per aggiungerne uno e Elimina per rimuovere quello selezionato. Fai clic su un fotogramma per selezionarlo e mostrarlo sul canvas. Il righello superiore indica il tempo in secondi.

4. MARCATORE VERDE: MODIFICARE LA DURATA
Sul marcatore verde al bordo sinistro del fotogramma, tieni premuto il tasto destro del mouse. Il cursore salta alla fine attuale del fotogramma; continua a trascinare per cambiare la durata e rilascia per confermare. Con una tavoletta, tocca con la punta della penna e mantieni premuto: non serve un tasto destro fisico. Puoi anche inserire la durata nel campo Dur. e premere Invio.

5. RIORDINARE I FOTOGRAMMI
Tieni premuto il tasto destro sul numero del fotogramma e trascinalo nella posizione desiderata. Con la penna, tieni la punta sul numero e trascina. Al rilascio i fotogrammi vengono riordinati e rinumerati. Num 4 e Num 6 spostano il fotogramma selezionato a sinistra o a destra quando il cursore è sulla timeline.

6. TESTINA DI RIPRODUZIONE E AUDIO
Trascina la testina sul righello superiore per scorrere il tempo. Se la riproduzione è attiva, iniziare a trascinare la testina ferma Play. Attiva Ascolta mentre sposti la testina per sentire l’audio durante la ricerca. Play avvia/mette in pausa l’anteprima e Loop ripete l’intervallo attivo.

7. STRUMENTI E SCORCIATOIE
Matita disegna; Gomma cancella; Secchiello riempie; Lazo seleziona; Ruota colori sceglie il colore; Contagocce preleva un colore; Render apre l’esportazione. Predefiniti: F1 preferenze; F2/F6 nuovo fotogramma; F3 Onion Skin; F4 render; F5 matita; F7 inserisci; F8 gomma; F9 secchiello; F10 ruota colori; F12 contagocce; virgola/precedente; punto/successivo; Tab massimizza/ripristina canvas; Invio riproduci/pausa se il cursore è sulla timeline; Delete elimina secondo l’area. Modifica i tasti in Scorciatoie.

8. COME FUNZIONA IL TASTO DESTRO
L’azione dipende dall’area sotto il cursore: sul canvas dipende dallo strumento/selezione; sulla timeline, tenere premuto il tasto destro trascina il marcatore verde per cambiare durata oppure il numero per riordinare. Sulla tavoletta basta tenere premuta la punta. Se una scorciatoia non funziona, controlla la posizione del cursore e che non sia attivo un campo di testo.

9. LIVELLI E ONION SKIN
Seleziona un livello nella colonna sinistra. L’icona di visibilità lo mostra o nasconde. Onion mostra i fotogrammi vicini come riferimento; regola l’opacità con il campo %. Aggiungere, eliminare o spostare livelli cambia l’organizzazione, non la risoluzione del canvas.

10. RISOLUZIONE, DPI E QUALITÀ
Larghezza e altezza determinano i pixel del disegno: più pixel offrono più dettagli ma richiedono memoria e possono rallentare modifica/render. DPI descrive la densità per dimensione fisica o stampa; aumentare DPI da solo non aggiunge dettagli in pixel. Scegli la risoluzione prima di disegnare.

11. RENDER / ESPORTAZIONE
Fai clic su Render nella barra strumenti o premi F4. Scegli formato, nome e cartella di uscita. Esporta MP4/H.264, GIF animato o MOV con video PNG; GIF/MOV possono usare trasparenza e MP4/MOV possono includere audio. MP4/H.264 richiede Windows 7+ e supporto Media Foundation/encoder H.264; su Windows XP usa GIF o MOV/PNG.

12. LINGUA E IMPOSTAZIONI
Nella schermata iniziale usa Language per scegliere la lingua. In Preferenze regola tema, dimensione interfaccia/testo, motore grafico, tavoletta, righe timeline, cronologia e scorciatoie. Alcune opzioni si applicano dopo il salvataggio.

NUOVE SCORCIATOIE TIMELINE
Num + aumenta la durata selezionata di 0,1 secondi; Num - la riduce rispettando il minimo. Num 4 sposta il fotogramma a sinistra, Num 6 a destra. Funzionano solo con il cursore sulla timeline. I gesti del marcatore verde e di riordino restano disponibili con tasto destro o pressione prolungata della penna.)KODA")},
        {L"ru", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — СПРАВКА

1. СОЗДАНИЕ, ОТКРЫТИЕ И СОХРАНЕНИЕ ПРОЕКТА
На стартовом экране выберите «Создать», чтобы задать имя, папку, ширину, высоту и FPS. Выберите «Открыть», чтобы продолжить существующий проект. Во время редактирования нажмите Ctrl+S для сохранения. Не отделяйте папку проекта от подпапки assets.

2. ХОЛСТ, РИСОВАНИЕ И НАВИГАЦИЯ
Холст — область рисования. Выберите инструмент на боковой панели и рисуйте мышью или пером. Удерживайте Пробел во время перетаскивания, чтобы перемещать вид, а не рисунок. Положение курсора и окно под ним влияют на срабатывание сочетаний клавиш.

3. ТАЙМЛАЙН И КАДРЫ
Таймлайн упорядочивает кадры по порядку и длительности. «+ Кадр» создаёт кадр, «Вставить» вставляет, «Удалить» удаляет выбранный. Щёлкните кадр, чтобы выбрать его и показать рисунок на холсте. Верхняя линейка отображает время в секундах.

4. ЗЕЛЁНЫЙ МАРКЕР: ИЗМЕНЕНИЕ ДЛИТЕЛЬНОСТИ
Удерживайте правую кнопку мыши на зелёном маркере у левого края кадра. Курсор переместится к текущему концу кадра; продолжайте перетаскивание, чтобы изменить длительность, затем отпустите кнопку. На планшете коснитесь кончиком пера и удерживайте — физическая правая кнопка не нужна. Можно также ввести длительность в поле Dur. и нажать Enter.

5. ПЕРЕУПОРЯДОЧИВАНИЕ КАДРОВ
Удерживайте правую кнопку на номере кадра и перетащите его в нужное место. Пером удерживайте кончик на номере и перетащите. После отпускания кадры перемещаются и нумеруются заново. Num 4 и Num 6 также перемещают выбранный кадр на одну позицию влево или вправо, когда курсор находится над таймлайном.

6. УКАЗАТЕЛЬ ВРЕМЕНИ И ЗВУК
Перетаскивайте указатель по верхней линейке, чтобы просматривать время. Если воспроизведение идёт, начало перетаскивания останавливает Play. Включите прослушивание при перемещении указателя, чтобы слышать звук во время поиска. Play запускает/останавливает просмотр, Loop повторяет активный диапазон.

7. ИНСТРУМЕНТЫ И СОЧЕТАНИЯ КЛАВИШ
Карандаш рисует; ластик стирает; заливка заполняет; лассо выделяет; цветовой круг выбирает цвет; пипетка берёт цвет с холста; Render открывает экспорт. По умолчанию: F1 настройки; F2/F6 новый кадр; F3 Onion Skin; F4 рендер; F5 карандаш; F7 вставка; F8 ластик; F9 заливка; F10 цветовой круг; F12 пипетка; запятая/предыдущий кадр; точка/следующий; Tab развернуть/восстановить холст; Enter воспроизведение/пауза, когда курсор на таймлайне; Delete удаляет в зависимости от области курсора. Изменяйте клавиши в разделе «Сочетания клавиш».

8. КАК РАБОТАЕТ ПРАВАЯ КНОПКА
Действие зависит от области под курсором: на холсте — от инструмента/выделения; на таймлайне удерживание правой кнопки перетаскивает зелёный маркер для изменения длительности или номер для перестановки. На планшете это заменяет длительное касание пером. Если сочетание не работает, проверьте положение курсора и не редактируется ли текстовое поле.

9. СЛОИ И ONION SKIN
Выберите слой в левой колонке. Значок видимости показывает или скрывает слой. Onion показывает соседние кадры как ориентир; прозрачность регулируется полем %. Добавление, удаление и перемещение слоёв меняет организацию, но не разрешение холста.

10. РАЗРЕШЕНИЕ, DPI И КАЧЕСТВО
Ширина и высота определяют количество пикселей: больше пикселей дают больше деталей, но используют больше памяти и могут замедлить редактирование/рендер. DPI описывает плотность для физического размера или печати; повышение DPI само по себе не добавляет пиксельных деталей. Выберите разрешение до рисования.

11. РЕНДЕР / ЭКСПОРТ
Нажмите Render на панели или F4. Выберите формат, имя и папку. Экспортируются MP4/H.264, анимированный GIF или MOV с PNG-видео; GIF/MOV поддерживают прозрачность, MP4/MOV могут включать звук. Для MP4/H.264 нужны Windows 7+ и Media Foundation/кодировщик H.264; в Windows XP используйте GIF или MOV/PNG.

12. ЯЗЫК И НАСТРОЙКИ
На стартовом экране выберите Language. В настройках можно изменить тему, масштаб интерфейса/текста, графический движок, планшет, высоту строк таймлайна, историю и сочетания клавиш. Некоторые параметры применяются после сохранения.

НОВЫЕ СОЧЕТАНИЯ ДЛЯ ТАЙМЛАЙНА
Num + увеличивает длительность выбранного кадра на 0,1 с; Num - уменьшает её до минимума. Num 4 перемещает кадр влево, Num 6 — вправо. Эти действия работают, когда курсор находится над таймлайном. Жесты зелёного маркера и перестановки доступны через правую кнопку или удержание пера.)KODA")},
        {L"zh", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — 使用说明

1. 新建、打开和保存项目
在启动界面选择“创建”来设置名称、文件夹、宽度、高度和 FPS。选择“打开”继续已有项目。编辑时按 Ctrl+S 保存。请保留项目文件夹及其 assets 子文件夹，不要将导入素材分开。

2. 画布、绘图和导航
画布是绘图区域。在侧边工具栏选择工具，用鼠标或数位笔绘画。拖动时按住空格键可移动视图，不会移动画稿。光标所在位置以及其下方的窗口会影响快捷键的作用。

3. 时间轴和帧
时间轴按顺序和持续时间组织帧。使用“+ 帧”创建帧、“插入”插入帧、“删除”移除选中帧。点击帧即可选中，并在画布上显示其绘图。顶部标尺以秒显示时间。

4. 绿色标记：调整帧时长
在帧左边缘的绿色标记上按住鼠标右键。光标会跳到该帧当前结束位置；继续拖动调整时长，松开确认。使用数位板时，用笔尖触碰并按住即可，不需要实体右键。也可以在 Dur. 字段输入时长并按 Enter。

5. 调整帧顺序
在帧编号上按住鼠标右键并拖到目标位置。使用数位笔时，在编号上按住笔尖并拖动。松开后帧会重新排序并编号。当光标位于时间轴上时，Num 4 和 Num 6 也可将选中帧向左或向右移动一格。

6. 播放头和音频
拖动顶部标尺上的播放头可浏览时间。播放时开始拖动播放头会停止 Play。启用“移动播放头时试听”，即可在定位时听到音频。Play 用于播放/暂停预览，Loop 用于重复启用的范围。

7. 工具和快捷键
铅笔绘画；橡皮擦除；油漆桶填充；套索选择；色轮选色；吸管从画布取色；渲染打开导出选项。默认快捷键：F1 设置；F2/F6 新建帧；F3 Onion Skin；F4 渲染；F5 铅笔；F7 插入帧；F8 橡皮；F9 油漆桶；F10 色轮；F12 吸管；逗号/上一帧；句点/下一帧；Tab 最大化/还原画布；光标在时间轴时按 Enter 播放/暂停；Delete 根据光标位置删除。可在“快捷键”中修改按键。

8. 鼠标右键的作用
操作取决于光标所在区域：在画布上取决于工具或选区；在时间轴上按住右键可拖动绿色标记调整时长，或拖动帧编号重新排序。数位板上长按笔尖可模拟这些操作。如果快捷键没有反应，请检查光标所在窗口以及是否正在编辑文本框。

9. 图层和 Onion Skin
在左侧列选择图层。可见性图标用于显示/隐藏图层。Onion 可显示相邻帧作为参考；用 % 字段调整不透明度。添加、删除或移动图层只改变组织方式，不改变画布分辨率。

10. 分辨率、DPI 和画质
宽度和高度决定画稿包含的像素数量：像素越多，细节越丰富，但更占内存，也可能降低编辑/渲染速度。DPI 表示实体尺寸或打印密度；单独提高 DPI 不会增加像素细节。建议在开始绘画前确定分辨率。

11. 渲染/导出
点击工具栏的“渲染”或按 F4。选择格式、文件名和输出文件夹。支持 MP4/H.264、动画 GIF 或带 PNG 视频的 MOV；GIF/MOV 可使用透明背景，MP4/MOV 可包含音频。MP4/H.264 需要 Windows 7 或更高版本以及 Media Foundation/H.264 编码器；Windows XP 请使用 GIF 或 MOV/PNG。

12. 语言和设置
在启动界面选择 Language 更改界面语言。在设置中可调整主题、界面/文字大小、图形引擎、数位板、时间轴行高、历史记录和快捷键。部分选项需保存设置后生效。

时间轴新增快捷键
Num + 将选中帧时长增加 0.1 秒；Num - 减少 0.1 秒，但不会低于最小值。Num 4 将帧向左移动，Num 6 向右移动。这些功能仅在光标位于时间轴上时生效。绿色标记和帧排序也可继续使用鼠标右键拖动或长按数位笔尖。)KODA")},
        {L"ja", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — 使い方の説明

1. プロジェクトの作成・読み込み・保存
開始画面で「作成」を選び、名前、フォルダー、幅、高さ、FPSを設定します。既存の作業は「開く」で読み込みます。編集中は Ctrl+S で保存します。読み込んだ素材を失わないよう、プロジェクトフォルダーと assets サブフォルダーを一緒に保管してください。

2. キャンバス、描画、操作
キャンバスは描画領域です。側面のツールバーでツールを選び、マウスまたはペンで描きます。Space を押しながらドラッグすると、絵を動かさず表示位置を移動できます。カーソルの位置と下にあるウィンドウによって、実行されるショートカットが変わります。

3. タイムラインとフレーム
タイムラインはフレームの順序と長さを管理します。「+ フレーム」で作成、「挿入」で追加、「削除」で選択フレームを削除します。フレームをクリックすると選択され、キャンバスに表示されます。上部の定規は秒単位の時間です。

4. 緑のマーカー：フレーム時間の調整
フレーム左端の緑のマーカー上でマウス右ボタンを押し続けます。カーソルが現在のフレーム終端へ移動します。そのままドラッグして長さを調整し、ボタンを離して確定します。ペンタブレットではペン先を触れたまま保持すればよく、物理的な右ボタンは不要です。Dur. 欄に時間を入力して Enter を押す方法もあります。

5. フレームの並べ替え
フレーム番号上でマウス右ボタンを押し続け、目的の位置までドラッグします。ペンでは番号上でペン先を押し続けてドラッグします。離すとフレームの順序と番号が更新されます。カーソルがタイムライン上にあるとき、Num 4 と Num 6 でも選択フレームを左右へ一つ移動できます。

6. 再生ヘッドと音声
上部の定規で再生ヘッドをドラッグすると時間を移動できます。再生中にドラッグを始めると Play が停止します。「再生ヘッド移動中に音声を聞く」を有効にすると、位置を探しながら音を確認できます。Play は再生/一時停止、Loop は有効範囲の繰り返しです。

7. ツールとショートカット
鉛筆は描画、消しゴムは消去、バケツは塗りつぶし、投げ縄は選択、カラーホイールは色選択、スポイトはキャンバスから色取得、Render は書き出しです。初期設定：F1 設定、F2/F6 新規フレーム、F3 Onion Skin、F4 レンダー、F5 鉛筆、F7 挿入、F8 消しゴム、F9 バケツ、F10 カラーホイール、F12 スポイト、カンマ/前のフレーム、ピリオド/次のフレーム、Tab キャンバスの最大化/復元、タイムライン上で Enter 再生/一時停止、Delete はカーソル位置に応じて削除。キーは「ショートカット」で変更できます。

8. 右ボタンの動作
操作はカーソル位置によって変わります。キャンバスではツール/選択に応じた操作、タイムラインでは右ボタンを押したまま緑のマーカーを動かして長さを変更したり、番号をドラッグして順序を変更したりできます。タブレットではペン先の長押しで同じ操作を行います。反応しない場合はカーソル位置とテキスト入力中でないかを確認してください。

9. レイヤーと Onion Skin
左側でレイヤーを選択します。表示アイコンで表示/非表示を切り替えます。Onion は前後のフレームを参考表示し、% 欄で不透明度を調整します。レイヤーの追加・削除・移動は構成を変えるだけで、キャンバス解像度は変わりません。

10. 解像度、DPI、画質
幅と高さは描画のピクセル数です。ピクセルが多いほど細部を表現できますが、メモリー使用量が増え、編集やレンダーが遅くなる場合があります。DPI は印刷などの物理的な密度を表し、DPI だけを上げてもピクセルの細部は増えません。描画前に解像度を決めましょう。

11. レンダー/書き出し
ツールバーの Render または F4 を押し、形式、名前、保存先を選びます。MP4/H.264、アニメーション GIF、PNG 動画の MOV に対応します。GIF/MOV は透明背景に対応し、MP4/MOV には音声を含められます。MP4/H.264 には Windows 7 以降と Media Foundation/H.264 エンコーダーが必要です。Windows XP では GIF または MOV/PNG を使ってください。

12. 言語と設定
開始画面の Language で言語を選択します。設定ではテーマ、UI/文字サイズ、描画エンジン、タブレット、タイムライン行の高さ、履歴、ショートカットを調整できます。一部の項目は設定保存後に反映されます。

タイムラインの新しいショートカット
Num + は選択フレームの長さを 0.1 秒増やし、Num - は最小値を下回らず 0.1 秒減らします。Num 4 は左へ、Num 6 は右へフレームを移動します。カーソルがタイムライン上にある場合のみ動作します。緑のマーカー調整と並べ替えは、右ボタンのドラッグまたはペン先の長押しでも使えます。)KODA")},
        {L"ko", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — 사용 설명

1. 프로젝트 만들기, 열기, 저장
시작 화면에서 만들기를 선택해 이름, 폴더, 너비, 높이, FPS를 설정합니다. 기존 작업은 열기를 선택합니다. 편집 중 Ctrl+S로 저장합니다. 가져온 미디어를 유지하려면 프로젝트 폴더와 assets 하위 폴더를 함께 보관하세요.

2. 캔버스, 그리기, 탐색
캔버스는 그림을 그리는 영역입니다. 측면 도구 모음에서 도구를 선택하고 마우스나 펜으로 그립니다. Space를 누른 채 드래그하면 그림 자체가 아니라 보기를 이동합니다. 커서 위치와 그 아래의 창에 따라 작동하는 단축키가 달라집니다.

3. 타임라인과 프레임
타임라인은 프레임의 순서와 길이를 관리합니다. + 프레임으로 만들고, 삽입으로 추가하고, 삭제로 선택된 프레임을 제거합니다. 프레임을 클릭하면 선택되어 캔버스에 표시됩니다. 위쪽 눈금자는 초 단위 시간을 나타냅니다.

4. 녹색 마커: 프레임 길이 조정
프레임 왼쪽 가장자리의 녹색 마커에서 마우스 오른쪽 버튼을 누르고 유지합니다. 커서가 현재 프레임 끝으로 이동합니다. 계속 드래그해 길이를 조정하고 놓으면 확정됩니다. 타블렛은 펜촉을 대고 누른 상태로 유지하면 되며 물리적인 오른쪽 버튼이 필요 없습니다. Dur. 필드에 시간을 입력하고 Enter를 눌러도 됩니다.

5. 프레임 순서 바꾸기
프레임 번호 위에서 마우스 오른쪽 버튼을 누른 채 원하는 위치로 드래그합니다. 펜을 사용할 때는 번호 위에 펜촉을 누른 채 드래그합니다. 놓으면 프레임 순서와 번호가 갱신됩니다. 커서가 타임라인 위에 있을 때 Num 4와 Num 6으로 선택 프레임을 좌우 한 칸 이동할 수도 있습니다.

6. 재생 헤드와 오디오
위쪽 눈금자에서 재생 헤드를 드래그해 시간을 이동합니다. 재생 중 드래그를 시작하면 Play가 중지됩니다. 재생 헤드 이동 중 오디오 듣기를 켜면 위치를 찾으면서 소리를 들을 수 있습니다. Play는 재생/일시정지, Loop는 활성 구간 반복입니다.

7. 도구와 단축키
연필은 그리기, 지우개는 지우기, 버킷은 채우기, 올가미는 선택, 색상 휠은 색 선택, 스포이드는 캔버스 색 추출, 렌더는 내보내기입니다. 기본값: F1 설정; F2/F6 새 프레임; F3 Onion Skin; F4 렌더; F5 연필; F7 삽입; F8 지우개; F9 버킷; F10 색상 휠; F12 스포이드; 쉼표/이전 프레임; 마침표/다음 프레임; Tab 캔버스 최대화/복원; 타임라인 위에서 Enter 재생/일시정지; Delete는 커서 위치에 따라 삭제합니다. 단축키에서 변경할 수 있습니다.

8. 마우스 오른쪽 버튼의 동작
동작은 커서 아래 영역에 따라 달라집니다. 캔버스에서는 도구/선택 상태에 맞는 명령을 사용하고, 타임라인에서는 오른쪽 버튼을 누른 채 녹색 마커를 드래그해 길이를 바꾸거나 번호를 드래그해 순서를 바꿉니다. 타블렛은 펜촉을 길게 누르면 같은 동작을 합니다. 단축키가 작동하지 않으면 커서 위치와 텍스트 편집 여부를 확인하세요.

9. 레이어와 Onion Skin
왼쪽 목록에서 레이어를 선택합니다. 표시 아이콘으로 레이어를 보이거나 숨깁니다. Onion은 이전/다음 프레임을 참고로 표시하며 % 필드로 불투명도를 조절합니다. 레이어 추가/삭제/이동은 구성을 바꾸지만 캔버스 해상도는 바꾸지 않습니다.

10. 해상도, DPI, 품질
너비와 높이는 그림의 픽셀 수입니다. 픽셀이 많으면 세부 묘사가 좋아지지만 메모리를 더 사용하고 편집/렌더가 느려질 수 있습니다. DPI는 인쇄 등 물리적 크기를 위한 밀도이며 DPI만 올려도 픽셀 세부 정보가 늘지 않습니다. 그리기 전에 해상도를 정하세요.

11. 렌더/내보내기
도구 모음의 렌더를 클릭하거나 F4를 누릅니다. 형식, 파일 이름, 출력 폴더를 선택합니다. MP4/H.264, 애니메이션 GIF, PNG 비디오 MOV를 지원합니다. GIF/MOV는 투명도를 사용할 수 있고 MP4/MOV에는 오디오를 포함할 수 있습니다. MP4/H.264는 Windows 7 이상과 Media Foundation/H.264 인코더가 필요합니다. Windows XP에서는 GIF 또는 MOV/PNG를 사용하세요.

12. 언어 및 설정
시작 화면의 Language에서 언어를 선택합니다. 설정에서 테마, UI/글자 크기, 그래픽 엔진, 타블렛, 타임라인 행 높이, 기록, 단축키를 조절합니다. 일부 옵션은 설정을 저장해야 적용됩니다.

타임라인 새 단축키
Num +는 선택 프레임 길이를 0.1초 늘리고 Num -는 최소값까지 줄입니다. Num 4는 프레임을 왼쪽으로, Num 6은 오른쪽으로 이동합니다. 커서가 타임라인 위에 있을 때만 작동합니다. 녹색 마커 조정과 순서 변경은 오른쪽 버튼 드래그 또는 펜촉 길게 누르기로도 가능합니다.)KODA")},
        {L"ar", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — دليل الاستخدام

1. إنشاء المشاريع وفتحها وحفظها
من شاشة البداية اختر إنشاء لتحديد الاسم والمجلد والعرض والارتفاع وFPS. اختر فتح لمتابعة مشروع موجود. اضغط Ctrl+S أثناء التحرير للحفظ. احتفظ بمجلد المشروع والمجلد الفرعي assets معًا حتى لا تفقد الوسائط المستوردة.

2. اللوحة والرسم والتنقل
اللوحة هي مساحة الرسم. اختر أداة من الشريط الجانبي وارسم بالفأرة أو القلم. اضغط Space أثناء السحب لتحريك العرض دون تحريك الرسم نفسه. يؤثر موضع المؤشر والنافذة الموجودة تحته في الاختصارات التي تعمل.

3. الخط الزمني والإطارات
ينظم الخط الزمني الإطارات حسب الترتيب والمدة. استخدم + إطار لإنشاء إطار، وInsert للإدراج، وDelete لإزالة الإطار المحدد. انقر إطارًا لتحديده وعرض رسمه على اللوحة. يعرض المسطرة العلوية الوقت بالثواني.

4. العلامة الخضراء: تغيير المدة
اضغط باستمرار على زر الفأرة الأيمن فوق العلامة الخضراء عند الحافة اليسرى للإطار. ينتقل المؤشر إلى نهاية الإطار الحالية؛ واصل السحب لتغيير المدة ثم اترك الزر للتأكيد. مع الجهاز اللوحي، المس بالقلم وابقِ رأسه مضغوطًا؛ لا تحتاج إلى زر أيمن فعلي. يمكنك أيضًا كتابة المدة في حقل Dur. والضغط على Enter.

5. إعادة ترتيب الإطارات
اضغط باستمرار على الزر الأيمن فوق رقم الإطار واسحبه إلى المكان المطلوب. باستخدام القلم، أبقِ رأس القلم مضغوطًا فوق الرقم واسحب. عند الإفلات يعاد ترتيب الإطارات وترقيمها. ينقل Num 4 وNum 6 الإطار المحدد خطوة لليسار أو اليمين عندما يكون المؤشر فوق الخط الزمني.

6. رأس التشغيل والصوت
اسحب رأس التشغيل على المسطرة العلوية للتنقل عبر الزمن. إذا كان التشغيل نشطًا، فإن بدء السحب يوقف Play. فعّل الاستماع أثناء تحريك رأس التشغيل لسماع الصوت أثناء البحث. يشغل Play المعاينة أو يوقفها مؤقتًا، ويكرر Loop النطاق المفعّل.

7. الأدوات والاختصارات
القلم يرسم؛ الممحاة تمسح؛ الدلو يملأ؛ اللاسو يحدد؛ عجلة الألوان تختار اللون؛ القطّارة تلتقط لونًا من اللوحة؛ Render يفتح التصدير. الافتراضي: F1 الإعدادات؛ F2/F6 إطار جديد؛ F3 Onion Skin؛ F4 التصيير؛ F5 القلم؛ F7 إدراج؛ F8 الممحاة؛ F9 الدلو؛ F10 عجلة الألوان؛ F12 القطّارة؛ الفاصلة/الإطار السابق؛ النقطة/التالي؛ Tab تكبير/استعادة اللوحة؛ Enter تشغيل/إيقاف مؤقت عندما يكون المؤشر فوق الخط الزمني؛ Delete يحذف بحسب المنطقة. يمكن تغيير المفاتيح في قسم الاختصارات.

8. وظيفة زر الفأرة الأيمن
تعتمد العملية على المنطقة تحت المؤشر: على اللوحة تعتمد على الأداة/التحديد؛ وعلى الخط الزمني يؤدي الضغط المطول على الزر الأيمن إلى سحب العلامة الخضراء لتغيير المدة أو رقم الإطار لتغيير الترتيب. في الجهاز اللوحي يحاكي لمس القلم المطول هذه الإيماءة. إذا لم يعمل اختصار، تحقق من موضع المؤشر ومن أنك لا تعدّل حقل نص.

9. الطبقات وOnion Skin
حدد طبقة من العمود الأيسر. أيقونة الرؤية تُظهر الطبقة أو تخفيها. يعرض Onion الإطارات المجاورة كمرجع؛ اضبط العتامة من حقل %. إضافة الطبقات أو حذفها أو نقلها يغيّر التنظيم لا دقة اللوحة.

10. الدقة وDPI والجودة
يحدد العرض والارتفاع عدد البكسلات: المزيد من البكسلات يعطي تفاصيل أكثر لكنه يستهلك الذاكرة وقد يبطئ التحرير والتصيير. يصف DPI كثافة الحجم المادي أو الطباعة؛ رفع DPI وحده لا يضيف تفاصيل بكسلية. اختر الدقة قبل الرسم.

11. التصيير والتصدير
انقر Render في شريط الأدوات أو اضغط F4. اختر التنسيق والاسم ومجلد الإخراج. يدعم البرنامج MP4/H.264 وGIF متحركًا وMOV بفيديو PNG؛ يمكن لـGIF/MOV استخدام الشفافية ويمكن لـMP4/MOV تضمين الصوت. يتطلب MP4/H.264 نظام Windows 7 أو أحدث ودعم Media Foundation/مرمّز H.264؛ على Windows XP استخدم GIF أو MOV/PNG.

12. اللغة والإعدادات
من شاشة البداية اختر Language. في الإعدادات يمكنك تغيير السمة وحجم الواجهة/النص ومحرك الرسوم والجهاز اللوحي وارتفاع صفوف الخط الزمني والسجل والاختصارات. تسري بعض الخيارات بعد حفظ الإعدادات.

اختصارات الخط الزمني الجديدة
يزيد Num + مدة الإطار المحدد بمقدار 0.1 ثانية، ويقللها Num - مع احترام الحد الأدنى. ينقل Num 4 الإطار لليسار وNum 6 لليمين. تعمل هذه الأوامر فقط عندما يكون المؤشر فوق الخط الزمني. وتظل إيماءات العلامة الخضراء وإعادة الترتيب متاحة بالضغط على الزر الأيمن أو إبقاء رأس القلم مضغوطًا.)KODA")},
        {L"hi", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — उपयोग मार्गदर्शिका

1. प्रोजेक्ट बनाना, खोलना और सहेजना
शुरुआती स्क्रीन पर Create चुनकर नाम, फ़ोल्डर, चौड़ाई, ऊँचाई और FPS निर्धारित करें। पुराने प्रोजेक्ट के लिए Open चुनें। संपादन के दौरान Ctrl+S दबाकर सहेजें। आयात किए गए मीडिया को बनाए रखने के लिए प्रोजेक्ट फ़ोल्डर और assets उपफ़ोल्डर साथ रखें।

2. कैनवास, ड्रॉइंग और नेविगेशन
कैनवास ड्रॉइंग क्षेत्र है। साइड टूलबार से टूल चुनें और माउस या पेन से बनाएँ। Space दबाकर ड्रैग करने से चित्र नहीं, दृश्य स्थान बदलता है। कर्सर की जगह और उसके नीचे की विंडो तय करती है कि कौन-से शॉर्टकट काम करेंगे।

3. टाइमलाइन और फ़्रेम
टाइमलाइन फ़्रेमों को क्रम और अवधि के अनुसार व्यवस्थित करती है। + Frame से नया फ़्रेम बनाएँ, Insert से डालें और Delete से चुना फ़्रेम हटाएँ। किसी फ़्रेम पर क्लिक कर उसे चुनें और कैनवास पर देखें। ऊपर की रूलर समय को सेकंड में दिखाती है।

4. हरा मार्कर: अवधि बदलना
फ़्रेम के बाएँ किनारे पर हरे मार्कर पर माउस का दायाँ बटन दबाकर रखें। कर्सर फ़्रेम के मौजूदा अंत पर चला जाएगा; अवधि बदलने के लिए ड्रैग जारी रखें और पुष्टि के लिए छोड़ दें। टैबलेट पर पेन की नोक दबाकर रखें—भौतिक दायाँ बटन आवश्यक नहीं। Dur. फ़ील्ड में अवधि लिखकर Enter भी दबा सकते हैं।

5. फ़्रेम का क्रम बदलना
फ़्रेम संख्या पर दायाँ बटन दबाकर उसे इच्छित स्थान तक ड्रैग करें। पेन से संख्या पर नोक दबाकर ड्रैग करें। छोड़ने पर फ़्रेम का क्रम और नंबर अपडेट होंगे। टाइमलाइन पर कर्सर होने पर Num 4 और Num 6 चुने फ़्रेम को एक स्थान बाएँ या दाएँ भी ले जाते हैं।

6. प्लेहेड और ऑडियो
ऊपरी रूलर पर प्लेहेड ड्रैग करके समय खोजें। प्लेबैक चल रहा हो तो प्लेहेड खींचना शुरू करते ही Play रुक जाता है। खोजते समय ऑडियो सुनने के लिए Scrub audio with playhead चालू करें। Play पूर्वावलोकन चलाता/रोकता है और Loop सक्रिय रेंज दोहराता है।

7. टूल और शॉर्टकट
Pencil बनाता है; Eraser मिटाता है; Bucket भरता है; Lasso चयन करता है; Color wheel रंग चुनता है; Eyedropper कैनवास से रंग लेता है; Render निर्यात खोलता है। डिफ़ॉल्ट: F1 सेटिंग; F2/F6 नया फ़्रेम; F3 Onion Skin; F4 Render; F5 Pencil; F7 Insert; F8 Eraser; F9 Bucket; F10 रंग चक्र; F12 Eyedropper; कॉमा/पिछला फ़्रेम; पीरियड/अगला; Tab कैनवास बड़ा/बहाल; टाइमलाइन पर कर्सर होने पर Enter चलाएँ/रोकें; Delete कर्सर क्षेत्र के अनुसार हटाता है। Shortcuts में कुंजियाँ बदलें।

8. दायाँ माउस बटन कैसे काम करता है
क्रिया कर्सर के नीचे के क्षेत्र पर निर्भर करती है: कैनवास पर टूल/चयन के अनुसार; टाइमलाइन पर दायाँ बटन दबाकर हरा मार्कर खींचने से अवधि और संख्या खींचने से क्रम बदलता है। टैबलेट पर पेन की नोक लंबे समय तक दबाना यही क्रिया करता है। शॉर्टकट न चले तो कर्सर की जगह और टेक्स्ट फ़ील्ड की स्थिति देखें।

9. लेयर और Onion Skin
बाएँ कॉलम में लेयर चुनें। दृश्यता आइकन उसे दिखाता/छिपाता है। Onion पड़ोसी फ़्रेमों को संदर्भ के रूप में दिखाता है; % फ़ील्ड से अपारदर्शिता बदलें। लेयर जोड़ने, हटाने या स्थान बदलने से संगठन बदलता है, कैनवास रिज़ॉल्यूशन नहीं।

10. रिज़ॉल्यूशन, DPI और गुणवत्ता
चौड़ाई और ऊँचाई पिक्सेल की संख्या तय करती हैं: अधिक पिक्सेल अधिक बारीक विवरण देते हैं, पर अधिक मेमोरी लेते हैं और संपादन/रेंडर धीमा कर सकते हैं। DPI भौतिक आकार/प्रिंट की घनता है; केवल DPI बढ़ाने से पिक्सेल विवरण नहीं बढ़ता। ड्रॉइंग से पहले रिज़ॉल्यूशन चुनें।

11. रेंडर/निर्यात
टूलबार में Render क्लिक करें या F4 दबाएँ। फ़ॉर्मेट, नाम और आउटपुट फ़ोल्डर चुनें। MP4/H.264, एनिमेटेड GIF या PNG वीडियो वाला MOV निर्यात कर सकते हैं; GIF/MOV में पारदर्शिता और MP4/MOV में ऑडियो शामिल हो सकता है। MP4/H.264 के लिए Windows 7+ और Media Foundation/H.264 एन्कोडर चाहिए; Windows XP पर GIF या MOV/PNG उपयोग करें।

12. भाषा और सेटिंग
शुरुआती स्क्रीन पर Language से भाषा चुनें। सेटिंग में थीम, इंटरफ़ेस/टेक्स्ट आकार, ग्राफ़िक्स इंजन, टैबलेट, टाइमलाइन पंक्ति ऊँचाई, इतिहास और शॉर्टकट बदलें। कुछ विकल्प सेव करने के बाद लागू होते हैं।

टाइमलाइन के नए शॉर्टकट
Num + चुने फ़्रेम की अवधि 0.1 सेकंड बढ़ाता है; Num - न्यूनतम सीमा तक घटाता है। Num 4 फ़्रेम को बाएँ और Num 6 दाएँ ले जाता है। ये केवल टाइमलाइन पर कर्सर होने पर काम करते हैं। हरे मार्कर और पुनःक्रमण के लिए दायाँ माउस बटन या पेन की नोक दबाकर रखने का इशारा भी काम करता है।)KODA")},
        {L"id", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — PANDUAN PENGGUNA

1. MEMBUAT, MEMBUKA, DAN MENYIMPAN PROYEK
Di layar awal, pilih Buat untuk menentukan nama, folder, lebar, tinggi, dan FPS. Pilih Buka untuk melanjutkan proyek yang ada. Tekan Ctrl+S saat mengedit untuk menyimpan. Simpan folder proyek dan subfolder assets bersama agar media impor tidak hilang.

2. KANVAS, MENGGAMBAR, DAN NAVIGASI
Kanvas adalah area menggambar. Pilih alat pada bilah samping dan gambar dengan mouse atau pena. Tahan Space saat menyeret untuk menggeser tampilan tanpa memindahkan gambar. Posisi kursor dan jendela di bawahnya memengaruhi pintasan yang dijalankan.

3. TIMELINE DAN FRAME
Timeline mengatur frame menurut urutan dan durasi. Gunakan + Frame untuk membuat, Insert untuk menyisipkan, dan Delete untuk menghapus frame yang dipilih. Klik frame untuk memilih dan menampilkan gambarnya di kanvas. Penggaris atas menunjukkan waktu dalam detik.

4. PENANDA HIJAU: MENGUBAH DURASI
Tahan tombol kanan mouse pada penanda hijau di tepi kiri frame. Kursor melompat ke akhir frame saat ini; lanjutkan menyeret untuk mengubah durasi lalu lepaskan untuk mengonfirmasi. Dengan tablet, sentuh menggunakan ujung pena dan tahan—tidak perlu tombol kanan fisik. Durasi juga bisa diketik pada kolom Dur. lalu tekan Enter.

5. MENGURUTKAN ULANG FRAME
Tahan tombol kanan mouse pada nomor frame dan seret ke posisi yang diinginkan. Dengan pena, tahan ujung pena di atas nomor lalu seret. Saat dilepas, urutan dan nomor frame diperbarui. Num 4 dan Num 6 juga memindahkan frame terpilih satu posisi ke kiri atau kanan saat kursor berada di timeline.

6. PLAYHEAD DAN AUDIO
Seret playhead pada penggaris atas untuk menelusuri waktu. Jika pemutaran aktif, mulai menyeret playhead akan menghentikan Play. Aktifkan dengarkan audio saat menggeser playhead untuk mendengar audio saat mencari posisi. Play memulai/menjeda pratinjau dan Loop mengulang rentang aktif.

7. ALAT DAN PINTASAN
Pensil menggambar; Penghapus menghapus; Ember mengisi; Lasso memilih; Roda warna memilih warna; Pipet mengambil warna dari kanvas; Render membuka ekspor. Default: F1 preferensi; F2/F6 frame baru; F3 Onion Skin; F4 render; F5 pensil; F7 sisipkan frame; F8 penghapus; F9 ember; F10 roda warna; F12 pipet; koma/frame sebelumnya; titik/frame berikutnya; Tab maksimalkan/pulihkan kanvas; Enter putar/jeda saat kursor di timeline; Delete menghapus sesuai area kursor. Ubah tombol pada bagian Pintasan.

8. CARA KERJA TOMBOL KANAN
Aksi bergantung pada area di bawah kursor: pada kanvas mengikuti alat/pilihan; pada timeline, menahan tombol kanan lalu menyeret penanda hijau mengubah durasi, sedangkan menyeret nomor mengubah urutan. Pada tablet, menahan ujung pena meniru gestur ini. Jika pintasan tidak bekerja, periksa lokasi kursor dan apakah kolom teks sedang diedit.

9. LAYER DAN ONION SKIN
Pilih layer pada kolom kiri. Ikon visibilitas menampilkan atau menyembunyikannya. Onion menampilkan frame sekitar sebagai referensi; ubah opasitas dengan kolom %. Menambah, menghapus, atau memindahkan layer mengubah organisasi, bukan resolusi kanvas.

10. RESOLUSI, DPI, DAN KUALITAS
Lebar dan tinggi menentukan jumlah piksel gambar: lebih banyak piksel memberi detail lebih banyak, tetapi memakai lebih banyak memori dan dapat memperlambat edit/render. DPI menggambarkan kepadatan untuk ukuran fisik/cetak; menaikkan DPI saja tidak menambah detail piksel. Tentukan resolusi sebelum menggambar.

11. RENDER / EKSPOR
Klik Render pada bilah alat atau tekan F4. Pilih format, nama, dan folder keluaran. Format yang didukung: MP4/H.264, GIF animasi, atau MOV dengan video PNG; GIF/MOV dapat memakai transparansi dan MP4/MOV dapat menyertakan audio. MP4/H.264 memerlukan Windows 7+ dan Media Foundation/encoder H.264; pada Windows XP gunakan GIF atau MOV/PNG.

12. BAHASA DAN PENGATURAN
Pada layar awal pilih Language. Di Preferensi, ubah tema, ukuran antarmuka/teks, mesin grafis, tablet, tinggi baris timeline, riwayat, dan pintasan. Beberapa opsi berlaku setelah pengaturan disimpan.

PINTASAN TIMELINE BARU
Num + menambah durasi frame terpilih 0,1 detik; Num - menguranginya hingga batas minimum. Num 4 memindahkan frame ke kiri dan Num 6 ke kanan. Fungsi ini hanya berjalan saat kursor berada di timeline. Gestur penanda hijau dan pengurutan tetap bisa dilakukan dengan seret tombol kanan atau menahan ujung pena.)KODA")},
        {L"tr", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — KULLANIM AÇIKLAMASI

1. PROJE OLUŞTURMA, AÇMA VE KAYDETME
Başlangıç ekranında ad, klasör, genişlik, yükseklik ve FPS ayarlamak için Oluştur’u seçin. Var olan projeyi sürdürmek için Aç’ı seçin. Düzenlerken Ctrl+S ile kaydedin. İçe aktarılan medyanın kaybolmaması için proje klasörünü ve assets alt klasörünü birlikte tutun.

2. KANVAS, ÇİZİM VE GEZİNME
Canvas çizim alanıdır. Yan araç çubuğundan bir araç seçip fare veya kalemle çizin. Çizimi değil görünümü kaydırmak için sürüklerken Space tuşunu basılı tutun. İmlecin konumu ve altındaki pencere hangi kısayolların çalışacağını etkiler.

3. ZAMAN ÇİZELGESİ VE KARELER
Zaman çizelgesi kareleri sıra ve süreye göre düzenler. + Kare ile oluşturun, Ekle ile araya ekleyin, Sil ile seçili kareyi kaldırın. Bir kareye tıklayarak seçin ve çizimini kanvasta görüntüleyin. Üst cetvel zamanı saniye olarak gösterir.

4. YEŞİL İŞARET: SÜREYİ DEĞİŞTİRME
Karenin sol kenarındaki yeşil işarette farenin sağ tuşunu basılı tutun. İmleç karenin mevcut sonuna atlar; yeni süreyi ayarlamak için sürüklemeye devam edip onaylamak üzere bırakın. Tablet kullanırken kalem ucuna dokunup basılı tutmanız yeterlidir; fiziksel sağ tuş gerekmez. Süreyi Dur. alanına yazıp Enter’a da basabilirsiniz.

5. KARELERİ YENİDEN SIRALAMA
Kare numarasının üzerinde sağ tuşu basılı tutup istediğiniz konuma sürükleyin. Kalemle numaranın üzerinde ucu basılı tutup sürükleyin. Bırakınca kareler yeniden sıralanır ve numaralanır. İmleç zaman çizelgesindeyken Num 4 ve Num 6 seçili kareyi bir konum sola veya sağa taşır.

6. OYNATMA KAFASI VE SES
Zaman içinde gezinmek için üst cetvelde oynatma kafasını sürükleyin. Oynatma sürüyorsa sürüklemeye başlamak Play’i durdurur. Arama sırasında ses duymak için oynatma kafasını taşırken ses dinlemeyi etkinleştirin. Play önizlemeyi başlatır/duraklatır, Loop etkin aralığı tekrarlar.

7. ARAÇLAR VE KISAYOLLAR
Kalem çizer; Silgi siler; Kova doldurur; Kement seçer; Renk çarkı renk seçer; Damlalık kanvastan renk alır; Render dışa aktarmayı açar. Varsayılanlar: F1 tercihler; F2/F6 yeni kare; F3 Onion Skin; F4 render; F5 kalem; F7 kare ekle; F8 silgi; F9 kova; F10 renk çarkı; F12 damlalık; virgül/önceki kare; nokta/sonraki kare; Tab kanvası büyüt/geri yükle; imleç zaman çizelgesindeyken Enter oynat/duraklat; Delete konuma göre siler. Tuşları Kısayollar bölümünde değiştirebilirsiniz.

8. SAĞ TUŞUN İŞLEYİŞİ
İşlem imlecin altındaki alana bağlıdır: kanvasta araç/seçime, zaman çizelgesinde ise sağ tuşa basılı tutup sürüklemek yeşil işaretle süreyi veya kare numarasıyla sıralamayı değiştirir. Tablette kalem ucunu basılı tutmak aynı hareketi yapar. Kısayol çalışmazsa imleç yerini ve metin alanının düzenlenip düzenlenmediğini kontrol edin.

9. KATMANLAR VE ONION SKIN
Soldaki sütundan katman seçin. Görünürlük simgesi katmanı gösterir/gizler. Onion komşu kareleri referans olarak gösterir; opaklığı % alanından ayarlayın. Katman ekleme, silme veya taşıma düzeni değiştirir, kanvas çözünürlüğünü değil.

10. ÇÖZÜNÜRLÜK, DPI VE KALİTE
Genişlik ve yükseklik çizimin piksel sayısını belirler: daha fazla piksel daha fazla ayrıntı sağlar ancak daha çok bellek kullanır ve düzenleme/render işlemini yavaşlatabilir. DPI fiziksel boyut/baskı yoğunluğudur; yalnızca DPI artırmak piksel ayrıntısını artırmaz. Çizime başlamadan çözünürlüğü seçin.

11. RENDER / DIŞA AKTARMA
Araç çubuğundaki Render’a tıklayın veya F4’e basın. Biçimi, adı ve çıktı klasörünü seçin. MP4/H.264, animasyonlu GIF veya PNG videolu MOV dışa aktarılır; GIF/MOV şeffaflık kullanabilir, MP4/MOV ses içerebilir. MP4/H.264 için Windows 7+ ve Media Foundation/H.264 kodlayıcı desteği gerekir; Windows XP’de GIF veya MOV/PNG kullanın.

12. DİL VE AYARLAR
Başlangıç ekranında Language ile dili seçin. Tercihlerde tema, arayüz/metin boyutu, grafik motoru, tablet, zaman çizelgesi satır yüksekliği, geçmiş ve kısayolları ayarlayın. Bazı seçenekler ayarları kaydedince uygulanır.

YENİ ZAMAN ÇİZELGESİ KISAYOLLARI
Num + seçili karenin süresini 0,1 saniye artırır; Num - en düşük sınıra kadar azaltır. Num 4 kareyi sola, Num 6 sağa taşır. Bu işlemler yalnızca imleç zaman çizelgesindeyken çalışır. Yeşil işaret ve sıralama hareketleri sağ tuşla sürükleme veya kalem ucunu basılı tutmayla da yapılabilir.)KODA")},
        {L"pl", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — OBJAŚNIENIE

1. TWORZENIE, OTWIERANIE I ZAPIS PROJEKTÓW
Na ekranie startowym wybierz Utwórz, aby ustawić nazwę, folder, szerokość, wysokość i FPS. Wybierz Otwórz, aby kontynuować istniejący projekt. Podczas edycji naciśnij Ctrl+S. Zachowaj folder projektu i podfolder assets razem, aby nie utracić zaimportowanych mediów.

2. PŁÓTNO, RYSOWANIE I NAWIGACJA
Płótno to obszar rysowania. Wybierz narzędzie z bocznego paska i rysuj myszą lub piórkiem. Przytrzymaj Spację podczas przeciągania, aby przesuwać widok, a nie rysunek. Położenie kursora i okno pod nim wpływają na działanie skrótów.

3. OŚ CZASU I KLATKI
Oś czasu porządkuje klatki według kolejności i czasu trwania. Użyj + Klatka, aby utworzyć klatkę, Wstaw, aby ją dodać, i Usuń, aby usunąć zaznaczoną. Kliknij klatkę, aby ją wybrać i wyświetlić na płótnie. Górna linijka pokazuje czas w sekundach.

4. ZIELONY ZNACZNIK: ZMIANA CZASU TRWANIA
Przytrzymaj prawy przycisk myszy na zielonym znaczniku przy lewej krawędzi klatki. Kursor przeskoczy na aktualny koniec klatki; przeciągaj dalej, aby zmienić czas, i puść, aby zatwierdzić. Na tablecie dotknij końcówką piórka i przytrzymaj ją — fizyczny prawy przycisk nie jest potrzebny. Możesz też wpisać czas w polu Dur. i nacisnąć Enter.

5. ZMIANA KOLEJNOŚCI KLATEK
Przytrzymaj prawy przycisk myszy na numerze klatki i przeciągnij go w żądane miejsce. Piórkiem przytrzymaj końcówkę na numerze i przeciągnij. Po puszczeniu klatki zostaną uporządkowane i ponumerowane ponownie. Num 4 i Num 6 przesuwają zaznaczoną klatkę o jedną pozycję w lewo lub w prawo, gdy kursor znajduje się na osi czasu.

6. GŁOWICA ODTWARZANIA I DŹWIĘK
Przeciągaj głowicę na górnej linijce, aby przeglądać czas. Jeśli odtwarzanie działa, rozpoczęcie przeciągania zatrzymuje Play. Włącz odsłuch podczas przesuwania głowicy, aby słyszeć dźwięk podczas szukania. Play odtwarza/wstrzymuje podgląd, Loop powtarza aktywny zakres.

7. NARZĘDZIA I SKRÓTY
Ołówek rysuje; Gumka ściera; Wiadro wypełnia; Lasso zaznacza; Koło kolorów wybiera kolor; Pipeta pobiera kolor z płótna; Render otwiera eksport. Domyślne: F1 ustawienia; F2/F6 nowa klatka; F3 Onion Skin; F4 render; F5 ołówek; F7 wstaw; F8 gumka; F9 wiadro; F10 koło kolorów; F12 pipeta; przecinek/poprzednia klatka; kropka/następna klatka; Tab maksymalizuj/przywróć płótno; Enter odtwarzaj/wstrzymaj, gdy kursor jest na osi czasu; Delete usuwa zależnie od obszaru. Klawisze zmienisz w Skrótach.

8. DZIAŁANIE PRAWEGO PRZYCISKU
Operacja zależy od obszaru pod kursorem: na płótnie od narzędzia/zaznaczenia; na osi czasu przytrzymanie prawego przycisku i przeciągnięcie zielonego znacznika zmienia czas, a przeciągnięcie numeru zmienia kolejność. Na tablecie przytrzymanie końcówki piórka wykonuje ten sam gest. Jeśli skrót nie działa, sprawdź pozycję kursora i czy nie edytujesz pola tekstowego.

9. WARSTWY I ONION SKIN
Wybierz warstwę w lewej kolumnie. Ikona widoczności pokazuje ją lub ukrywa. Onion wyświetla sąsiednie klatki jako odniesienie; krycie ustawisz polem %. Dodawanie, usuwanie i przesuwanie warstw zmienia organizację, a nie rozdzielczość płótna.

10. ROZDZIELCZOŚĆ, DPI I JAKOŚĆ
Szerokość i wysokość określają liczbę pikseli: więcej pikseli oznacza więcej szczegółów, ale większe zużycie pamięci i wolniejszą edycję/renderowanie. DPI opisuje gęstość dla rozmiaru fizycznego lub druku; samo zwiększenie DPI nie dodaje szczegółów pikseli. Wybierz rozdzielczość przed rysowaniem.

11. RENDER / EKSPORT
Kliknij Render na pasku narzędzi lub naciśnij F4. Wybierz format, nazwę i folder wyjściowy. Dostępne są MP4/H.264, animowany GIF i MOV z wideo PNG; GIF/MOV obsługują przezroczystość, MP4/MOV mogą zawierać dźwięk. MP4/H.264 wymaga Windows 7+ oraz Media Foundation/kodeka H.264; w Windows XP użyj GIF lub MOV/PNG.

12. JĘZYK I USTAWIENIA
Na ekranie startowym wybierz Language. W Preferencjach zmień motyw, rozmiar interfejsu/tekstu, silnik grafiki, tablet, wysokość wierszy osi czasu, historię i skróty. Niektóre opcje działają po zapisaniu ustawień.

NOWE SKRÓTY OSI CZASU
Num + zwiększa czas zaznaczonej klatki o 0,1 s; Num - zmniejsza go do minimum. Num 4 przesuwa klatkę w lewo, Num 6 w prawo. Działają tylko, gdy kursor znajduje się na osi czasu. Gesty zielonego znacznika i zmiany kolejności nadal działają przez przeciąganie prawym przyciskiem lub długie przytrzymanie piórka.)KODA")},
        {L"nl", KodaNormalizarQuebrasAjuda(LR"KODA(KODANIMA 2D — UITLEG

1. PROJECTEN MAKEN, OPENEN EN OPSLAAN
Kies op het startscherm Maken om naam, map, breedte, hoogte en FPS in te stellen. Kies Openen om een bestaand project voort te zetten. Druk tijdens het bewerken op Ctrl+S om op te slaan. Bewaar de projectmap en de submap assets bij elkaar.

2. CANVAS, TEKENEN EN NAVIGATIE
De canvas is het tekengebied. Kies een gereedschap op de zijbalk en teken met muis of pen. Houd Spatie ingedrukt tijdens slepen om het beeld te verschuiven zonder de tekening te verplaatsen. De cursorpositie en het venster eronder bepalen welke sneltoetsen werken.

3. TIJDLIJN EN FRAMES
De tijdlijn ordent frames op volgorde en duur. Gebruik + Frame om te maken, Invoegen om in te voegen en Verwijderen om het geselecteerde frame te verwijderen. Klik op een frame om het te selecteren en op de canvas weer te geven. De bovenste liniaal toont seconden.

4. GROENE MARKER: DUUR WIJZIGEN
Houd de rechtermuisknop ingedrukt op de groene marker aan de linkerrand van het frame. De cursor springt naar het huidige einde van het frame; sleep verder om de duur te wijzigen en laat los om te bevestigen. Op een tekentablet houd je de penpunt ingedrukt; een fysieke rechtermuisknop is niet nodig. Je kunt de duur ook invullen in Dur. en op Enter drukken.

5. FRAMES HERSCHIKKEN
Houd de rechtermuisknop op het framenummer ingedrukt en sleep naar de gewenste plek. Met een pen houd je de punt op het nummer en sleep je. Bij loslaten worden frames opnieuw geordend en genummerd. Num 4 en Num 6 verplaatsen het geselecteerde frame ook één plek naar links of rechts als de cursor boven de tijdlijn staat.

6. AFSPEELKOP EN AUDIO
Sleep de afspeelkop over de bovenste liniaal om door de tijd te zoeken. Als afspelen actief is, stopt het zodra je de afspeelkop gaat slepen. Schakel audio beluisteren tijdens scrubben in om geluid te horen tijdens het zoeken. Play start/pauzeert de preview en Loop herhaalt het actieve bereik.

7. GEREEDSCHAPPEN EN SNELTOETSEN
Potlood tekent; Gum wist; Emmer vult; Lasso selecteert; Kleurenwiel kiest kleur; Pipet neemt een kleur van de canvas; Render opent export. Standaard: F1 voorkeuren; F2/F6 nieuw frame; F3 Onion Skin; F4 render; F5 potlood; F7 invoegen; F8 gum; F9 emmer; F10 kleurenwiel; F12 pipet; komma/vorige frame; punt/volgende frame; Tab canvas maximaliseren/herstellen; Enter afspelen/pauzeren wanneer de cursor op de tijdlijn staat; Delete verwijdert afhankelijk van het gebied. Pas toetsen aan bij Sneltoetsen.

8. WERKING VAN DE RECHTERMUISKNOP
De actie hangt af van het gebied onder de cursor: op de canvas van het gereedschap/de selectie; op de tijdlijn verandert slepen met ingedrukte rechtermuisknop via de groene marker de duur of via het framenummer de volgorde. Op een tablet doet een lang ingedrukte penpunt hetzelfde. Controleer bij een niet-werkende sneltoets de cursorpositie en of je geen tekstveld bewerkt.

9. LAGEN EN ONION SKIN
Selecteer een laag in de linkerkolom. Het zichtbaarheidsicoon toont of verbergt de laag. Onion toont naburige frames als referentie; pas de dekking aan met het %-veld. Lagen toevoegen, verwijderen of verplaatsen verandert de organisatie, niet de canvasresolutie.

10. RESOLUTIE, DPI EN KWALITEIT
Breedte en hoogte bepalen het aantal pixels: meer pixels geven meer detail, maar gebruiken meer geheugen en kunnen bewerken/renderen vertragen. DPI beschrijft dichtheid voor fysieke grootte/afdrukken; alleen DPI verhogen voegt geen pixeldetail toe. Kies de resolutie voordat je tekent.

11. RENDEREN / EXPORTEREN
Klik op Render op de werkbalk of druk op F4. Kies formaat, naam en uitvoermap. Export is mogelijk naar MP4/H.264, geanimeerde GIF of MOV met PNG-video; GIF/MOV kunnen transparantie gebruiken en MP4/MOV kunnen audio bevatten. MP4/H.264 vereist Windows 7+ en Media Foundation/H.264-encoderondersteuning; gebruik op Windows XP GIF of MOV/PNG.

12. TAAL EN INSTELLINGEN
Kies Language op het startscherm. Bij Voorkeuren kun je thema, interface-/tekstgrootte, grafische engine, tablet, tijdlijnrijhoogte, geschiedenis en sneltoetsen aanpassen. Sommige opties werken na het opslaan van instellingen.

NIEUWE TIJDLIJN-SNELTOETSEN
Num + verhoogt de duur van het geselecteerde frame met 0,1 seconde; Num - verlaagt die tot de minimumduur. Num 4 verplaatst het frame naar links en Num 6 naar rechts. Deze acties werken alleen als de cursor boven de tijdlijn staat. De groene-marker- en herschikgebaren blijven beschikbaar met slepen met rechts of een langdurige pentouch.)KODA")}
    };
    const std::wstring lang=KodaIdiomaBase();
    std::map<std::wstring,std::wstring>::const_iterator it=docs.find(lang);
    if(it!=docs.end()) return it->second;
    // Os idiomas ainda sem pacote completo recebem a versão inglesa em vez
    // de misturar frases sem tradução com conteúdo técnico em português.
    return docs.find(L"en")->second;
}
static std::wstring KodaRotuloExplicacao(bool upper=false) {
    const std::wstring lang=KodaIdiomaBase();
    // Nos idiomas ainda sem tradução completa, mantenha o mesmo fallback inglês
    // usado pelo conteúdo de ajuda em vez de deixar o título em português.
    const wchar_t* label=(lang==L"pt")?L"EXPLICAÇÃO":L"EXPLANATION";
    if(lang==L"en") label=L"EXPLANATION";
    else if(lang==L"es") label=L"EXPLICACIÓN";
    else if(lang==L"fr") label=L"EXPLICATION";
    else if(lang==L"de") label=L"ERKLÄRUNG";
    else if(lang==L"it") label=L"SPIEGAZIONE";
    else if(lang==L"ru") label=L"СПРАВКА";
    else if(lang==L"zh") label=L"使用说明";
    else if(lang==L"ja") label=L"使い方の説明";
    else if(lang==L"ko") label=L"사용 설명";
    else if(lang==L"ar") label=L"دليل الاستخدام";
    else if(lang==L"hi") label=L"उपयोग मार्गदर्शिका";
    else if(lang==L"id") label=L"PANDUAN PENGGUNA";
    else if(lang==L"tr") label=L"KULLANIM AÇIKLAMASI";
    else if(lang==L"pl") label=L"OBJAŚNIENIE";
    else if(lang==L"nl") label=L"UITLEG";
    (void)upper;
    return label;
}

static LRESULT CALLBACK KodaCaptionCleanupSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR subclassId, DWORD_PTR refData) {
    if(msg==WM_NCDESTROY) {
        wchar_t* original=(wchar_t*)GetPropW(hwnd,L"Kodanima.OriginalCaption");
        if(original) { RemovePropW(hwnd,L"Kodanima.OriginalCaption"); delete[] original; }
        RemoveWindowSubclass(hwnd,KodaCaptionCleanupSubclass,subclassId);
    }
    return DefSubclassProc(hwnd,msg,wp,lp);
}
static std::wstring KodaTextoCanonico(const std::wstring& current);
static void KodaGuardarLegendaOriginal(HWND hwnd, LPCWSTR className, LPCWSTR caption, DWORD style) {
    if(!hwnd || !caption || !caption[0] || GetPropW(hwnd,L"Kodanima.OriginalCaption")) return;
    wchar_t cls[80]={0}; GetClassNameW(hwnd,cls,79);
    const bool label=(lstrcmpiW(cls,L"STATIC")==0 || lstrcmpiW(cls,L"BUTTON")==0);
    const bool topLevel=(style&WS_CHILD)==0;
    if(!label && !topLevel) return;
    size_t n=wcslen(caption); wchar_t* copy=new(std::nothrow) wchar_t[n+1];
    if(!copy) return; memcpy(copy,caption,(n+1)*sizeof(wchar_t));
    if(!SetPropW(hwnd,L"Kodanima.OriginalCaption",(HANDLE)copy) || !SetWindowSubclass(hwnd,KodaCaptionCleanupSubclass,0x4B44,0)) {
        RemovePropW(hwnd,L"Kodanima.OriginalCaption"); delete[] copy;
    }
}
static HWND KodaCreateWindowW(LPCWSTR cls,LPCWSTR caption,DWORD style,int x,int y,int w,int h,HWND parent,HMENU menu,HINSTANCE inst,LPVOID param) {
    std::wstring source=caption?caption:L"";
    source=KodaTextoCanonico(source);
    std::wstring translated=KodaTraduzirTexto(source.c_str());
    HWND hwnd=::CreateWindowExW(0,cls,translated.c_str(),style,x,y,w,h,parent,menu,inst,param);
    KodaGuardarLegendaOriginal(hwnd,cls,source.c_str(),style); return hwnd;
}
static HWND KodaCreateWindowExW(DWORD exStyle,LPCWSTR cls,LPCWSTR caption,DWORD style,int x,int y,int w,int h,HWND parent,HMENU menu,HINSTANCE inst,LPVOID param) {
    std::wstring source=caption?caption:L"";
    source=KodaTextoCanonico(source);
    std::wstring translated=KodaTraduzirTexto(source.c_str());
    HWND hwnd=::CreateWindowExW(exStyle,cls,translated.c_str(),style,x,y,w,h,parent,menu,inst,param);
    KodaGuardarLegendaOriginal(hwnd,cls,source.c_str(),style); return hwnd;
}
static BOOL KodaSetWindowTextW(HWND hwnd, LPCWSTR caption) {
    if(!hwnd || !caption) return ::SetWindowTextW(hwnd,caption);
    wchar_t cls[80]={0}; GetClassNameW(hwnd,cls,79);
    const DWORD style=(DWORD)GetWindowLongPtrW(hwnd,GWL_STYLE);
    const bool translateLabel=(lstrcmpiW(cls,L"STATIC")==0 || lstrcmpiW(cls,L"BUTTON")==0 || (style&WS_CHILD)==0);
    if(!translateLabel || !caption[0]) return ::SetWindowTextW(hwnd,caption);
    // Se o chamador repassar um rótulo que já estava traduzido, recupera a chave
    // original antes de gravar a legenda, permitindo trocar de idioma repetidamente.
    std::wstring canonical=KodaTextoCanonico(caption);
    wchar_t* previous=(wchar_t*)GetPropW(hwnd,L"Kodanima.OriginalCaption");
    if(previous) { RemovePropW(hwnd,L"Kodanima.OriginalCaption"); delete[] previous; }
    KodaGuardarLegendaOriginal(hwnd,cls,canonical.c_str(),style);
    std::wstring translated=KodaTraduzirTexto(canonical.c_str());
    return ::SetWindowTextW(hwnd,translated.c_str());
}
static std::wstring KodaTextoCanonico(const std::wstring& current) {
    const std::wstring lang=KodaIdiomaBase();
    const auto& all=KodaCatalogoTraducao();
    for(const auto& row:all) {
        if(row.first==current) return row.first;
        for(const auto& tr:row.second) if(tr.second==current) return row.first;
    }
    const auto& ext=KodaCatalogoExtraTraducao();
    for(const auto& row:ext) {
        if(row.first==current) return row.first;
        for(const auto& tr:row.second) if(tr.second==current) return row.first;
    }
    const auto& supplemental=KodaCatalogoUiSupplement();
    for(const auto& row:supplemental) {
        if(row.first==current) return row.first;
        for(const auto& tr:row.second) if(tr.second==current) return row.first;
    }
    return current;
}
static void KodaAplicarIdiomaItens(HWND hwnd) {
    if(!hwnd || !IsWindow(hwnd)) return;
    wchar_t cls[32]={0}; GetClassNameW(hwnd,cls,31);
    if(lstrcmpiW(cls,L"ComboBox")!=0 && lstrcmpiW(cls,L"ListBox")!=0) return;
    if(lstrcmpiW(cls,L"ListBox")==0) {
        DWORD st=(DWORD)GetWindowLongPtrW(hwnd,GWL_STYLE);
        if(st&(LBS_OWNERDRAWFIXED|LBS_OWNERDRAWVARIABLE)) return; // preserva a biblioteca de nomes personalizados de pincéis
    }
    const bool combo=lstrcmpiW(cls,L"ComboBox")==0;
    int count=(int)SendMessageW(hwnd,combo?CB_GETCOUNT:LB_GETCOUNT,0,0);
    if(count<=0 || count>500) return;
    int selection=(int)SendMessageW(hwnd,combo?CB_GETCURSEL:LB_GETCURSEL,0,0);
    struct Item { std::wstring text; LPARAM data; };
    std::vector<Item> items; items.reserve((size_t)count);
    for(int i=0;i<count;++i) {
        int len=(int)SendMessageW(hwnd,combo?CB_GETLBTEXTLEN:LB_GETTEXTLEN,(WPARAM)i,0);
        if(len<0 || len>2048) continue;
        std::vector<wchar_t> buf((size_t)len+2,0);
        SendMessageW(hwnd,combo?CB_GETLBTEXT:LB_GETTEXT,(WPARAM)i,(LPARAM)buf.data());
        LPARAM data=SendMessageW(hwnd,combo?CB_GETITEMDATA:LB_GETITEMDATA,(WPARAM)i,0);
        items.push_back({KodaTextoCanonico(buf.data()),data});
    }
    SendMessageW(hwnd,combo?CB_RESETCONTENT:LB_RESETCONTENT,0,0);
    for(const auto& it:items) {
        std::wstring trans=KodaTraduzirTexto(it.text.c_str());
        LRESULT row=SendMessageW(hwnd,combo?CB_ADDSTRING:LB_ADDSTRING,0,(LPARAM)trans.c_str());
        if(row>=0) SendMessageW(hwnd,combo?CB_SETITEMDATA:LB_SETITEMDATA,(WPARAM)row,it.data);
    }
    if(selection>=0 && selection<(int)items.size()) SendMessageW(hwnd,combo?CB_SETCURSEL:LB_SETCURSEL,(WPARAM)selection,0);
}
static BOOL CALLBACK KodaAplicarIdiomaFilho(HWND hwnd, LPARAM) {
    wchar_t* original=(wchar_t*)GetPropW(hwnd,L"Kodanima.OriginalCaption");
    if(original) { std::wstring translated=KodaTraduzirTexto(original); ::SendMessageW(hwnd,WM_SETTEXT,0,(LPARAM)translated.c_str()); }
    KodaAplicarIdiomaItens(hwnd);
    return TRUE;
}
static BOOL CALLBACK KodaAplicarIdiomaRaiz(HWND hwnd, LPARAM) {
    KodaAplicarIdiomaFilho(hwnd,0);
    EnumChildWindows(hwnd,KodaAplicarIdiomaFilho,0);
    return TRUE;
}
static void KodaAtualizarIdiomaTodasJanelas() {
    EnumThreadWindows(GetCurrentThreadId(),KodaAplicarIdiomaRaiz,0);
    if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
    if(hwndTools) InvalidateRect(hwndTools,NULL,TRUE);
    if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,TRUE);
}

static LRESULT KodaSendMessageW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const bool isAdd=(msg==CB_ADDSTRING || msg==CB_INSERTSTRING || msg==LB_ADDSTRING || msg==LB_INSERTSTRING);
    if(isAdd && lp && IsWindow(hwnd)) {
        wchar_t cls[32]={0}; GetClassNameW(hwnd,cls,31);
        const bool combo=(lstrcmpiW(cls,L"ComboBox")==0);
        const bool list=(lstrcmpiW(cls,L"ListBox")==0);
        if(combo || list) {
            DWORD style=(DWORD)GetWindowLongPtrW(hwnd,GWL_STYLE);
            const bool ownerDraw=(combo ? ((style&(CBS_OWNERDRAWFIXED|CBS_OWNERDRAWVARIABLE))!=0)
                                        : ((style&(LBS_OWNERDRAWFIXED|LBS_OWNERDRAWVARIABLE))!=0));
            if(!ownerDraw) {
                const wchar_t* source=(const wchar_t*)lp;
                std::wstring translated=KodaTraduzirTexto(source);
                if(translated!=source) return ::SendMessageW(hwnd,msg,wp,(LPARAM)translated.c_str());
            }
        }
    }
    return ::SendMessageW(hwnd,msg,wp,lp);
}
static int KodaDrawTextW(HDC dc, LPCWSTR text, int count, LPRECT rect, UINT format) {
    if(!text || (format & DT_MODIFYSTRING)) return ::DrawTextW(dc,text,count,rect,format);
    std::wstring source;
    if(count < 0) source=text;
    else source.assign(text,text+(size_t)count);
    std::wstring translated=KodaTraduzirTexto(source.c_str());
    if(translated==source) return ::DrawTextW(dc,text,count,rect,format);
    return ::DrawTextW(dc,translated.c_str(),-1,rect,format);
}
static int KodaMessageBoxW(HWND owner, LPCWSTR message, LPCWSTR caption, UINT type) {
    std::wstring msg=message?KodaTraduzirTexto(KodaTextoCanonico(message).c_str()):L"";
    std::wstring cap=caption?KodaTraduzirTexto(KodaTextoCanonico(caption).c_str()):L"Kodanima";
    return ::MessageBoxW(owner,msg.c_str(),cap.c_str(),type);
}
static BOOL CALLBACK KodaTraduzirControleSeletorCor(HWND child, LPARAM) {
    wchar_t text[512]={0};
    if(!child || !IsWindow(child)) return TRUE;
    int len=GetWindowTextW(child,text,511);
    if(len>0) KodaSetWindowTextW(child,text);
    return TRUE;
}
static UINT_PTR CALLBACK KodaHookSeletorCor(HWND dialog, UINT message, WPARAM, LPARAM) {
    if(message==WM_INITDIALOG) {
        wchar_t title[256]={0};
        if(GetWindowTextW(dialog,title,255)>0) KodaSetWindowTextW(dialog,title);
        EnumChildWindows(dialog,KodaTraduzirControleSeletorCor,0);
    }
    return 0;
}

// Depois destes wrappers, todos os controles criados pelo programa passam pelo
// tradutor, sem modificar individualmente milhares de chamadas Win32.
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#ifdef CreateWindowExW
#undef CreateWindowExW
#endif
#define CreateWindowW KodaCreateWindowW
#define CreateWindowExW KodaCreateWindowExW
#ifdef SetWindowTextW
#undef SetWindowTextW
#endif
#define SetWindowTextW KodaSetWindowTextW
#ifdef SendMessageW
#undef SendMessageW
#endif
#define SendMessageW KodaSendMessageW
#ifdef DrawTextW
#undef DrawTextW
#endif
#define DrawTextW KodaDrawTextW
#ifdef MessageBoxW
#undef MessageBoxW
#endif
#define MessageBoxW KodaMessageBoxW

LRESULT CALLBACK WndProcLanguage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_COMMAND:
            if(LOWORD(wParam)==ID_LANGUAGE_APPLY) {
                HWND list=GetDlgItem(hwnd,ID_LANGUAGE_LIST);
                int selected=(int)SendMessageW(list,LB_GETCURSEL,0,0);
                if(selected!=LB_ERR) {
                    int idx=(int)SendMessageW(list,LB_GETITEMDATA,(WPARAM)selected,0);
                    if(idx>=0 && idx<(int)(sizeof(g_kodaLanguages)/sizeof(g_kodaLanguages[0]))) {
                        g_languageCode=g_kodaLanguages[idx].code;
                        SalvarConfiguracoesUsuario();
                        KodaAtualizarIdiomaTodasJanelas();
                    }
                }
                DestroyWindow(hwnd); return 0;
            }
            if(LOWORD(wParam)==ID_LANGUAGE_CANCEL) { DestroyWindow(hwnd); return 0; }
            break;
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_NCDESTROY: if(g_hwndLanguageDialog==hwnd) g_hwndLanguageDialog=NULL; break;
    }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}
static void AbrirSeletorIdioma(HWND owner) {
    if(g_hwndLanguageDialog && IsWindow(g_hwndLanguageDialog)) { SetForegroundWindow(g_hwndLanguageDialog); return; }
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=WndProcLanguage; wc.hInstance=hInstGlobal;
    wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512)); wc.hIcon=hIconeApp; wc.hIconSm=hIconeApp;
    wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1); wc.lpszClassName=L"KodanimaLanguageDialogClass";
    if(!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return;
    const std::wstring dialogTitle=KodaTraduzirTexto(L"Language settings");
    HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_TOOLWINDOW,L"KodanimaLanguageDialogClass",dialogTitle.c_str(),
        WS_CAPTION|WS_SYSMENU|WS_POPUP, CW_USEDEFAULT,CW_USEDEFAULT,520,540,owner,NULL,hInstGlobal,NULL);
    if(!h) return; g_hwndLanguageDialog=h;
    const std::wstring choosePrompt=KodaTraduzirTexto(L"Choose the interface language:");
    CreateWindowW(L"STATIC",choosePrompt.c_str(),WS_VISIBLE|WS_CHILD,18,16,470,24,h,(HMENU)ID_LANGUAGE_DESCRIPTION,hInstGlobal,NULL);
    HWND list=CreateWindowW(L"LISTBOX",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,
        18,45,468,410,h,(HMENU)ID_LANGUAGE_LIST,hInstGlobal,NULL);
    int chosen=0;
    const int count=(int)(sizeof(g_kodaLanguages)/sizeof(g_kodaLanguages[0]));
    for(int i=0;i<count;++i) {
        int row=(int)SendMessageW(list,LB_ADDSTRING,0,(LPARAM)g_kodaLanguages[i].nativeName);
        SendMessageW(list,LB_SETITEMDATA,(WPARAM)row,(LPARAM)i);
        if(g_languageCode==g_kodaLanguages[i].code) chosen=row;
    }
    SendMessageW(list,LB_SETCURSEL,(WPARAM)chosen,0);
    const std::wstring applyLabel=KodaTraduzirTexto(L"Apply");
    const std::wstring cancelLabel=KodaTraduzirTexto(L"Cancel");
    CreateWindowW(L"BUTTON",applyLabel.c_str(),WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,278,468,100,30,h,(HMENU)ID_LANGUAGE_APPLY,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",cancelLabel.c_str(),WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,386,468,100,30,h,(HMENU)ID_LANGUAGE_CANCEL,hInstGlobal,NULL);
    ShowWindow(h,SW_SHOWNORMAL); UpdateWindow(h); SetForegroundWindow(h);
}

// Funções de Manipulação de Bitmap e Cache
HBITMAP CopiarHBITMAP(HBITMAP hBmpSrc, int w, int h) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcSrc = CreateCompatibleDC(hdcScreen);
    HDC hdcDst = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmpDst = CreateCompatibleBitmap(hdcScreen, w, h);

    HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, hBmpSrc);
    HBITMAP hOldDst = (HBITMAP)SelectObject(hdcDst, hBmpDst);

    BitBlt(hdcDst, 0, 0, w, h, hdcSrc, 0, 0, SRCCOPY);

    SelectObject(hdcSrc, hOldSrc);
    SelectObject(hdcDst, hOldDst);
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
    ReleaseDC(NULL, hdcScreen);
    return hBmpDst;
}

static void LiberarTimelineSnapshot(TimelineSnapshot& snap) { for(auto& fr:snap.frames) for(HBITMAP b:fr.bitmaps) if(b) DeleteObject(b); snap.frames.clear(); }
static TimelineSnapshot CapturarTimelineSnapshot() {
    TimelineSnapshot snap; snap.layers=timelineLayers; snap.activeLayer=activeTimelineLayer; snap.selectedLayer=selectedTimelineLayer; snap.currentFrame=frameAtual;
    for(auto& f:listaFrames){ TLFrameSnapshot sf; sf.seconds=f.segundos; sf.durationUnit=f.unidadeDuracao; sf.layerDurations=f.layerDurations; sf.layerFrameExists=f.layerFrameExists; for(size_t i=0;i<f.layerBitmaps.size();++i) sf.bitmaps.push_back(f.layerBitmaps[i]?CopiarHBITMAP(f.layerBitmaps[i],configAtual.largura,configAtual.altura):NULL); snap.frames.push_back(std::move(sf)); }
    return snap;
}
static void LimparStackTimeline(std::vector<TimelineSnapshot>& st) { for(auto& x:st) LiberarTimelineSnapshot(x); st.clear(); }
static size_t TamanhoSnapshotTimeline(const TimelineSnapshot& snap) {
    size_t n=0;
    for(const auto& fr:snap.frames)
        n += fr.bitmaps.size()*(size_t)std::max(1,configAtual.largura)*(size_t)std::max(1,configAtual.altura)*4;
    return n;
}
static size_t MemoriaHistoricoTimelineTotal() {
    size_t n=0;
    for(const auto& x:timelineUndoStack) n+=TamanhoSnapshotTimeline(x);
    for(const auto& x:timelineRedoStack) n+=TamanhoSnapshotTimeline(x);
    return n;
}
static void AplicarLimitesHistoricoTimeline() {
    const size_t maxActions=(size_t)std::max(1,historyActionLimit);
    const size_t maxBytes=(size_t)std::max(1,historyCacheLimitMB)*1024u*1024u;
    auto totalActions=[&]() { return timelineUndoStack.size()+timelineRedoStack.size(); };
    size_t totalBytes=MemoriaHistoricoTimelineTotal();
    while((totalActions()>maxActions || totalBytes>maxBytes) &&
          (!timelineUndoStack.empty() || !timelineRedoStack.empty())) {
        // Descarta primeiro o histórico mais antigo; redo.front() é o redo mais distante.
        if(!timelineUndoStack.empty()) {
            totalBytes-=std::min(totalBytes,TamanhoSnapshotTimeline(timelineUndoStack.front()));
            LiberarTimelineSnapshot(timelineUndoStack.front());
            timelineUndoStack.erase(timelineUndoStack.begin());
        } else {
            totalBytes-=std::min(totalBytes,TamanhoSnapshotTimeline(timelineRedoStack.front()));
            LiberarTimelineSnapshot(timelineRedoStack.front());
            timelineRedoStack.erase(timelineRedoStack.begin());
        }
    }
}
static void RegistrarHistoricoTimeline() {
    if(timelineHistoryRestoring) return;
    // O snapshot tem o estado anterior à ação e precisa caber no orçamento configurado.
    size_t estimativa=listaFrames.size()*std::max<size_t>(1,timelineLayers.size())*
                      (size_t)std::max(1,configAtual.largura)*(size_t)std::max(1,configAtual.altura)*4;
    const size_t maxBytes=(size_t)std::max(1,historyCacheLimitMB)*1024u*1024u;
    if(estimativa > maxBytes) return;
    if(!timelineRedoStack.empty()) LimparStackTimeline(timelineRedoStack);
    timelineUndoStack.push_back(CapturarTimelineSnapshot());
    AplicarLimitesHistoricoTimeline();
}
static void RestaurarTimelineSnapshot(const TimelineSnapshot& snap) {
    timelineHistoryRestoring=true;
    for(auto& f:listaFrames) ExcluirBitmapsQuadro(f);
    InvalidarCacheComposicaoCanvas(); listaFrames.clear(); timelineLayers=snap.layers; if(timelineLayers.empty()) timelineLayers.push_back({L"Camada 1",true});
    for(const auto& sf:snap.frames){ FrameData f{}; f.id=(int)listaFrames.size()+1; f.segundos=sf.seconds; f.unidadeDuracao=sf.durationUnit; f.layerDurations=sf.layerDurations; f.layerFrameExists=sf.layerFrameExists; for(size_t i=0;i<timelineLayers.size();++i) f.layerBitmaps.push_back(i<sf.bitmaps.size()&&sf.bitmaps[i]?CopiarHBITMAP(sf.bitmaps[i],configAtual.largura,configAtual.altura):CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255))); f.hBmpQuadro=f.layerBitmaps.empty()?NULL:f.layerBitmaps[0]; listaFrames.push_back(std::move(f)); }
    activeTimelineLayer=std::max(0,std::min(snap.activeLayer,(int)timelineLayers.size()-1)); selectedTimelineLayer=std::max(0,std::min(snap.selectedLayer,(int)timelineLayers.size()-1)); frameAtual=std::max(0,std::min(snap.currentFrame,(int)listaFrames.size()-1)); selectedTimelineFrame=frameAtual; SincronizarAliasCamadaAtiva();
    if(hwndTimeline){ AtualizarCampoDuracao(hwndTimeline); InvalidateRect(hwndTimeline,NULL,TRUE); } InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); SalvarProjetoCompleto(); timelineHistoryRestoring=false;
}
static void DesfazerTimeline(bool redo) {
    std::vector<TimelineSnapshot>& from=redo?timelineRedoStack:timelineUndoStack;
    std::vector<TimelineSnapshot>& to=redo?timelineUndoStack:timelineRedoStack;
    if(from.empty()) return;
    size_t estimate=listaFrames.size()*std::max<size_t>(1,timelineLayers.size())*
                    (size_t)std::max(1,configAtual.largura)*(size_t)std::max(1,configAtual.altura)*4;
    const size_t maxBytes=(size_t)std::max(1,historyCacheLimitMB)*1024u*1024u;
    // O outro lado do histórico também precisa caber, para o redo/undo ser reversível.
    if(estimate>maxBytes) return;
    to.push_back(CapturarTimelineSnapshot());
    TimelineSnapshot target=std::move(from.back());
    from.pop_back();
    AplicarLimitesHistoricoTimeline();
    RestaurarTimelineSnapshot(target);
    LiberarTimelineSnapshot(target);
}

static size_t MemoriaHistoricoFrame(const FrameData& frame) {
    size_t n=0;
    for(const auto& st:frame.undoStack) n+=st.pixelData.size();
    for(const auto& st:frame.redoStack) n+=st.pixelData.size();
    return n;
}
static void AplicarLimitesHistoricoFrame(FrameData& frame) {
    const size_t maxActions=(size_t)std::max(1,historyActionLimit);
    const size_t maxBytes=(size_t)std::max(1,historyCacheLimitMB)*1024u*1024u;
    auto totalActions=[&]() { return frame.undoStack.size()+frame.redoStack.size(); };
    size_t totalBytes=MemoriaHistoricoFrame(frame);
    while((totalActions()>maxActions || totalBytes>maxBytes) &&
          (!frame.undoStack.empty() || !frame.redoStack.empty())) {
        if(!frame.undoStack.empty()) {
            totalBytes-=std::min(totalBytes,frame.undoStack.front().pixelData.size());
            frame.undoStack.pop_front();
        } else {
            totalBytes-=std::min(totalBytes,frame.redoStack.front().pixelData.size());
            frame.redoStack.pop_front();
        }
    }
    frame.cacheMemoryUsageBytes=MemoriaHistoricoFrame(frame);
}

void SalvarEstadoHistorico(FrameData& frame) {
    BITMAP bmp;
    GetObject(frame.hBmpQuadro, sizeof(BITMAP), &bmp);
    int datasize = ((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight;

    HistoryState state;
    state.largura = bmp.bmWidth;
    state.altura = bmp.bmHeight;
    state.pixelData.resize(datasize);

    HDC hdc = GetDC(NULL);
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    GetDIBits(hdc, frame.hBmpQuadro, 0, bmp.bmHeight, state.pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    frame.undoStack.push_back(std::move(state));
    frame.redoStack.clear(); // uma nova ação invalida a trilha de refazer
    AplicarLimitesHistoricoFrame(frame);
}

void ExecutarUndo() {
    if (frameAtual < 0 || frameAtual >= (int)listaFrames.size()) return;
    FrameData& f = listaFrames[frameAtual];
    if (!f.hBmpQuadro || f.undoStack.empty()) return;

    // Move atual para redo
    BITMAP bmp;
    GetObject(f.hBmpQuadro, sizeof(BITMAP), &bmp);
    HistoryState currentState;
    currentState.largura = bmp.bmWidth;
    currentState.altura = bmp.bmHeight;
    currentState.pixelData.resize(((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight);

    HDC hdc = GetDC(NULL);
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    GetDIBits(hdc, f.hBmpQuadro, 0, bmp.bmHeight, currentState.pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    f.redoStack.push_back(std::move(currentState));

    // Cada entrada guarda o estado ANTES de uma ação. Restaura exatamente
    // essa entrada e remove só uma ação do histórico.
    HistoryState prevState = f.undoStack.back();
    f.undoStack.pop_back();

    hdc = GetDC(NULL);
    SetDIBits(hdc, f.hBmpQuadro, 0, prevState.altura, prevState.pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    AplicarLimitesHistoricoFrame(f);
    InvalidarCacheComposicaoCanvas();
    if(hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
    if(hwndTimeline) InvalidateRect(hwndTimeline, NULL, TRUE);
    SalvarProjetoCompleto();
}

void ExecutarRedo() {
    if (frameAtual < 0 || frameAtual >= (int)listaFrames.size()) return;
    FrameData& f = listaFrames[frameAtual];
    if (!f.hBmpQuadro || f.redoStack.empty()) return;

    // Move atual para undo
    BITMAP bmp;
    GetObject(f.hBmpQuadro, sizeof(BITMAP), &bmp);
    HistoryState currentState;
    currentState.largura = bmp.bmWidth;
    currentState.altura = bmp.bmHeight;
    currentState.pixelData.resize(((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight);

    HDC hdc = GetDC(NULL);
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    GetDIBits(hdc, f.hBmpQuadro, 0, bmp.bmHeight, currentState.pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    f.undoStack.push_back(std::move(currentState));

    // Recupera estado do redo
    HistoryState nextState = f.redoStack.back();
    f.redoStack.pop_back();

    hdc = GetDC(NULL);
    SetDIBits(hdc, f.hBmpQuadro, 0, nextState.altura, nextState.pixelData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    AplicarLimitesHistoricoFrame(f);
    InvalidarCacheComposicaoCanvas();
    if(hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
    if(hwndTimeline) InvalidateRect(hwndTimeline, NULL, TRUE);
    SalvarProjetoCompleto();
}

// Diálogos Popups
INT_PTR CALLBACK DialogSegundosProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int* pVal = NULL;
    switch (message) {
        case WM_INITDIALOG:
            pVal = (int*)lParam;
            CreateWindowW(L"STATIC", L"Duração do quadro em segundos:", WS_VISIBLE | WS_CHILD, 20, 20, 220, 20, hDlg, NULL, hInstGlobal, NULL);
            CreateWindowW(L"EDIT", L"1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 20, 50, 220, 25, hDlg, (HMENU)1001, hInstGlobal, NULL);
            CreateWindowW(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 90, 90, 80, 30, hDlg, (HMENU)IDOK, hInstGlobal, NULL);
            SetDlgItemInt(hDlg, 1001, *pVal, FALSE);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                BOOL translated = FALSE;
                int val = GetDlgItemInt(hDlg, 1001, &translated, FALSE);
                if (translated && val > 0) *pVal = val;
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

INT_PTR CALLBACK DialogOpacidadeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int* pVal = NULL;
    switch (message) {
        case WM_INITDIALOG:
            pVal = (int*)lParam;
            CreateWindowW(L"STATIC", L"Opacidade Onion Skin (0-100%):", WS_VISIBLE | WS_CHILD, 20, 20, 220, 20, hDlg, NULL, hInstGlobal, NULL);
            CreateWindowW(L"EDIT", L"50", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 20, 50, 220, 25, hDlg, (HMENU)1002, hInstGlobal, NULL);
            CreateWindowW(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 90, 90, 80, 30, hDlg, (HMENU)IDOK, hInstGlobal, NULL);
            SetDlgItemInt(hDlg, 1002, *pVal, FALSE);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                BOOL translated = FALSE;
                int val = GetDlgItemInt(hDlg, 1002, &translated, FALSE);
                if (translated && val >= 0 && val <= 100) *pVal = val;
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

// Janela de exportação e renderizadores nativos do Kodanima.
#define ID_RENDER_FORMAT       7401
#define ID_RENDER_FOLDER       7402
#define ID_RENDER_NAME         7403
#define ID_RENDER_BROWSE       7404
#define ID_RENDER_ALPHA        7405
#define ID_RENDER_START        7406
#define ID_RENDER_CANCEL       7407
#define ID_RENDER_STATUS       7408

static HWND g_hwndRenderOptions = NULL;
static HWND g_hwndRenderProgress = NULL;
static bool g_renderProgressBusy = false;
static bool g_renderProgressHolding = false;
static bool g_renderProgressCompleted = false;
static size_t g_renderProgressFrame = 0;
static size_t g_renderProgressTotal = 1;
static DWORD g_renderProgressStarted = 0;
static std::wstring g_renderProgressFormat;
static std::wstring g_renderProgressStatus;

static void PumpRenderMessages() {
    MSG msg; int quitCode=-1;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { quitCode=(int)msg.wParam; break; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if(quitCode>=0) PostQuitMessage(quitCode);
}
static std::wstring TempoRenderTexto(unsigned long long s) {
    wchar_t b[96];
    if (s < 60) swprintf(b, 96, L"%llu s", s);
    else if (s < 3600) swprintf(b, 96, L"%llu min %llu s", s/60, s%60);
    else swprintf(b, 96, L"%llu h %llu min", s/3600, (s%3600)/60);
    return b;
}
static LRESULT CALLBACK WndProcRenderProgress(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_MOUSEMOVE) { InvalidateRect(h, NULL, FALSE); return 0; }
    if (m == WM_CLOSE && g_renderProgressBusy) return 0;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(h,&ps); RECT rc{}; GetClientRect(h,&rc);
        HBRUSH bg=CreateSolidBrush(RGB(246,247,250)); FillRect(dc,&rc,bg); DeleteObject(bg);
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(35,38,45));
        RECT title{20,12,rc.right-20,36}; { std::wstring titleText=KodaTraduzirTexto(L"Renderizando animação"); DrawTextW(dc,titleText.c_str(),-1,&title,DT_LEFT|DT_VCENTER|DT_SINGLELINE); }
        wchar_t line[256]={0};
        if(g_renderProgressCompleted) lstrcpynW(line,L"Concluído!",256);
        else if(g_renderProgressHolding) lstrcpynW(line,L"Finalizando arquivo...",256);
        else if(g_renderProgressTotal) swprintf(line,256,L"%s — Quadro %llu de %llu (%llu%%)",g_renderProgressFormat.c_str(),(unsigned long long)std::min(g_renderProgressFrame,g_renderProgressTotal),(unsigned long long)g_renderProgressTotal,(unsigned long long)(std::min(g_renderProgressFrame,g_renderProgressTotal)*100/g_renderProgressTotal));
        RECT status{20,42,rc.right-20,64}; DrawTextW(dc,line,-1,&status,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
        if(!g_renderProgressCompleted && !g_renderProgressHolding) {
            std::wstring eta=L"Calculando estimativa de tempo...";
            DWORD elapsed=(DWORD)(GetTickCount()-g_renderProgressStarted);
            if(g_renderProgressFrame>0 && g_renderProgressTotal>g_renderProgressFrame) {
                double secPerFrame=(double)elapsed/1000.0/(double)g_renderProgressFrame;
                unsigned long long remain=(unsigned long long)std::max(0.0,std::ceil(secPerFrame*(double)(g_renderProgressTotal-g_renderProgressFrame)));
                eta=L"Tempo estimado restante: "+TempoRenderTexto(remain);
            } else if(g_renderProgressFrame>=g_renderProgressTotal && g_renderProgressTotal>0) eta=L"Preparando fechamento do arquivo...";
            RECT etaRc{20,66,rc.right-20,86}; DrawTextW(dc,eta.c_str(),-1,&etaRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        } else if(g_renderProgressHolding) {
            RECT etaRc{20,66,rc.right-20,86}; DrawTextW(dc,L"Conferindo e finalizando os dados gravados...",-1,&etaRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        } else {
            RECT etaRc{20,66,rc.right-20,86}; DrawTextW(dc,L"O arquivo foi gerado com sucesso.",-1,&etaRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        }
        RECT bar{22,113,rc.right-22,139}; HBRUSH track=CreateSolidBrush(RGB(220,222,228)); FillRect(dc,&bar,track); DeleteObject(track);
        int barW=std::max(1,(int)(bar.right-bar.left)); double ratio=g_renderProgressTotal?(double)std::min(g_renderProgressFrame,g_renderProgressTotal)/(double)g_renderProgressTotal:0.0;
        if(g_renderProgressHolding) ratio=0.985; if(g_renderProgressCompleted) ratio=1.0;
        int filled=(int)std::lround(barW*std::max(0.0,std::min(1.0,ratio)));
        int halfway=barW/2;
        if(filled>0) { RECT red{bar.left,bar.top,bar.left+std::min(filled,halfway),bar.bottom}; HBRUSH rr=CreateSolidBrush(RGB(226,54,65)); FillRect(dc,&red,rr); DeleteObject(rr); }
        if(filled>halfway) { RECT green{bar.left+halfway,bar.top,bar.left+filled,bar.bottom}; HBRUSH gg=CreateSolidBrush(RGB(50,185,92)); FillRect(dc,&green,gg); DeleteObject(gg); }
        HPEN edge=CreatePen(PS_SOLID,1,RGB(150,153,163)); HGDIOBJ op=SelectObject(dc,edge); HGDIOBJ ob=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); Rectangle(dc,bar.left,bar.top,bar.right,bar.bottom); SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(edge);
        // O olho acompanha a extremidade do progresso; a pupila acompanha o mouse.
        int eyeX=(int)bar.left+filled; eyeX=std::max((int)bar.left+15,std::min((int)bar.right-15,eyeX)); int eyeY=99;
        HPEN eyePen=CreatePen(PS_SOLID,2,RGB(58,64,78)); HBRUSH eyeFill=CreateSolidBrush(RGB(255,255,255)); HGDIOBJ oldP=SelectObject(dc,eyePen); HGDIOBJ oldB=SelectObject(dc,eyeFill);
        Ellipse(dc,eyeX-14,eyeY-8,eyeX+14,eyeY+8); SelectObject(dc,oldB); DeleteObject(eyeFill);
        POINT mouse{}; GetCursorPos(&mouse); ScreenToClient(h,&mouse); int px=std::max(-5,std::min(5,(int)((mouse.x-eyeX)/4))); int py=std::max(-2,std::min(2,(int)((mouse.y-eyeY)/4)));
        HBRUSH pupil=CreateSolidBrush(RGB(24,28,36)); SelectObject(dc,pupil); Ellipse(dc,eyeX-4+px,eyeY-4+py,eyeX+4+px,eyeY+4+py); SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); DeleteObject(pupil); SelectObject(dc,oldP); DeleteObject(eyePen);
        EndPaint(h,&ps); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}
static void IniciarProgressoRender(HWND owner,size_t total,const wchar_t* format) {
    g_renderProgressFrame=0; g_renderProgressTotal=std::max((size_t)1,total); g_renderProgressStarted=GetTickCount();
    g_renderProgressFormat=format?format:L"Exportação"; g_renderProgressStatus.clear(); g_renderProgressHolding=false; g_renderProgressCompleted=false; g_renderProgressBusy=true;
    static bool registered=false;
    if(!registered) { WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=WndProcRenderProgress; wc.hInstance=hInstGlobal; wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512)); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); wc.lpszClassName=L"KodanimaRenderProgressClass"; RegisterClassExW(&wc); registered=true; }
    RECT wr{}; if(owner&&IsWindow(owner))GetWindowRect(owner,&wr); else SetRect(&wr,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
    int x=wr.left+(wr.right-wr.left-500)/2,y=wr.top+(wr.bottom-wr.top-190)/2;
    x=std::max(0,std::min(GetSystemMetrics(SM_CXSCREEN)-500,x)); y=std::max(0,std::min(GetSystemMetrics(SM_CYSCREEN)-190,y));
    g_hwndRenderProgress=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,L"KodanimaRenderProgressClass",L"Kodanima — Renderização",WS_CAPTION|WS_SYSMENU,x,y,500,190,owner,NULL,hInstGlobal,NULL);
    if(g_hwndRenderProgress){ ShowWindow(g_hwndRenderProgress,SW_SHOWNORMAL); UpdateWindow(g_hwndRenderProgress); }
    PumpRenderMessages();
}
static void AtualizarProgressoRender(size_t frame,size_t total) {
    g_renderProgressFrame=std::min(frame,std::max((size_t)1,total)); g_renderProgressTotal=std::max((size_t)1,total);
    if(g_hwndRenderProgress&&IsWindow(g_hwndRenderProgress)){InvalidateRect(g_hwndRenderProgress,NULL,FALSE);UpdateWindow(g_hwndRenderProgress);} PumpRenderMessages();
}
static void FinalizarProgressoRender(bool success) {
    // Fecha imediatamente a janela de progresso: esperas artificiais de 5 + 2 s
    // bloqueavam o aplicativo mesmo depois de a exportação já ter terminado.
    g_renderProgressBusy=false;
    g_renderProgressHolding=false;
    g_renderProgressCompleted=success;
    if(g_hwndRenderProgress&&IsWindow(g_hwndRenderProgress)) {
        InvalidateRect(g_hwndRenderProgress,NULL,FALSE);
        UpdateWindow(g_hwndRenderProgress);
        DestroyWindow(g_hwndRenderProgress);
    }
    g_hwndRenderProgress=NULL;
}
static bool ObterFaixaExportacao(double& start,double& finish,size_t& count,int& fps) {
    fps=configAtual.fps>0?configAtual.fps:24;
    if(listaFrames.empty()) return false;
    double total=TimelineSecondsTotalExact(); if(total<=0.001) total=(double)listaFrames.size()/fps;
    start=timelineLoopEnabled?std::max(0.0,std::min(total,timelineLoopStartSeconds)):0.0;
    finish=timelineLoopEnabled?std::max(start+0.001,std::min(total,timelineLoopEndSeconds)):total;
    if(finish<=start) finish=total;
    count=(size_t)std::max(1.0,std::ceil(std::max(0.001,finish-start)*fps));
    return true;
}
static bool ObterPixelsRGBA(HBITMAP bitmap,int& width,int& height,std::vector<BYTE>& rgba,bool transparentWhite) {
    if(!bitmap) return false; BITMAP bm{}; if(!GetObjectW(bitmap,sizeof(bm),&bm)||bm.bmWidth<=0||bm.bmHeight<=0) return false;
    width=(int)bm.bmWidth;height=(int)bm.bmHeight;
    if((unsigned long long)width*(unsigned long long)height>40000000ULL) return false;
    BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=width; bi.bmiHeader.biHeight=height; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
    std::vector<BYTE> bgra((size_t)width*(size_t)height*4); HDC dc=GetDC(NULL); int lines=GetDIBits(dc,bitmap,0,(UINT)height,bgra.data(),&bi,DIB_RGB_COLORS); ReleaseDC(NULL,dc); if(lines==0)return false;
    rgba.resize(bgra.size());
    for(int y=0;y<height;++y) {
        const BYTE* src=&bgra[(size_t)(height-1-y)*width*4];
        BYTE* dst=&rgba[(size_t)y*width*4];
#if KODA_GNU_X86_SIMD
        if(g_kodaCPU.pixelKernel==2) KodaConverterLinhaAVX2(src,dst,width,transparentWhite);
        else if(g_kodaCPU.pixelKernel==1) KodaConverterLinhaSSE2(src,dst,width,transparentWhite);
        else
#endif
        {
            for(int x=0;x<width;++x) {
                BYTE b=src[x*4],g=src[x*4+1],r=src[x*4+2];
                bool transparent=transparentWhite && r>=250 && g>=250 && b>=250;
                dst[x*4]=r; dst[x*4+1]=g; dst[x*4+2]=b; dst[x*4+3]=transparent?0:255;
            }
        }
    }
    return true;
}
static void AppendBE16(std::vector<BYTE>& v,unsigned int x){v.push_back((BYTE)(x>>8));v.push_back((BYTE)x);}
static void AppendBE32(std::vector<BYTE>& v,unsigned long x){v.push_back((BYTE)(x>>24));v.push_back((BYTE)(x>>16));v.push_back((BYTE)(x>>8));v.push_back((BYTE)x);}
static void AppendBE64(std::vector<BYTE>& v,unsigned long long x){AppendBE32(v,(unsigned long)(x>>32));AppendBE32(v,(unsigned long)x);}
static void AppendLE16(std::vector<BYTE>& v,unsigned int x){v.push_back((BYTE)x);v.push_back((BYTE)(x>>8));}
static void AppendLE32(std::vector<BYTE>& v,unsigned long x){v.push_back((BYTE)x);v.push_back((BYTE)(x>>8));v.push_back((BYTE)(x>>16));v.push_back((BYTE)(x>>24));}
static std::vector<BYTE> CriarAtomMov(const char* type,const std::vector<BYTE>& payload) {
    std::vector<BYTE> a; a.reserve(payload.size()+8); AppendBE32(a,(unsigned long)(payload.size()+8)); for(int i=0;i<4;++i)a.push_back((BYTE)type[i]); a.insert(a.end(),payload.begin(),payload.end()); return a;
}
static void AdicionarAtomMov(std::vector<BYTE>& parent,const char* type,const std::vector<BYTE>& payload) { std::vector<BYTE> a=CriarAtomMov(type,payload); parent.insert(parent.end(),a.begin(),a.end()); }
static bool EscreverHandle(HANDLE h,const void* data,size_t n) {
    const BYTE* p=(const BYTE*)data; while(n) { DWORD chunk=(DWORD)std::min((size_t)0x7ffff000,n); DWORD wrote=0; if(!WriteFile(h,p,chunk,&wrote,NULL)||wrote!=chunk)return false; p+=wrote;n-=wrote; } return true;
}
static bool EscreverByteHandle(HANDLE h,BYTE b){DWORD n=0;return WriteFile(h,&b,1,&n,NULL)&&n==1;}
static bool PosicaoHandle(HANDLE h,unsigned long long& pos){LARGE_INTEGER zero{},out{};if(!SetFilePointerEx(h,zero,&out,FILE_CURRENT))return false;pos=(unsigned long long)out.QuadPart;return true;}
static unsigned long CRC32PNG(const BYTE* p,size_t n) { unsigned long c=0xffffffffUL; for(size_t i=0;i<n;++i){c^=p[i];for(int k=0;k<8;++k)c=(c&1)?(c>>1)^0xedb88320UL:c>>1;} return c^0xffffffffUL; }
static unsigned long Adler32PNG(const BYTE* p,size_t n) { unsigned long a=1,b=0; for(size_t i=0;i<n;++i){a=(a+p[i])%65521UL;b=(b+a)%65521UL;}return (b<<16)|a; }
static void PNGChunk(std::vector<BYTE>& png,const char* type,const std::vector<BYTE>& data) {
    AppendBE32(png,(unsigned long)data.size()); size_t start=png.size(); for(int i=0;i<4;++i)png.push_back((BYTE)type[i]); png.insert(png.end(),data.begin(),data.end()); AppendBE32(png,CRC32PNG(&png[start],4+data.size()));
}
static bool CriarPNG_RGBA(int w,int h,const std::vector<BYTE>& rgba,std::vector<BYTE>& png) {
    if(w<=0||h<=0||rgba.size()!=(size_t)w*h*4||(unsigned long long)w*h>40000000ULL)return false;
    std::vector<BYTE> raw; raw.reserve((size_t)h*((size_t)w*4+1)); for(int y=0;y<h;++y){raw.push_back(0);const BYTE* row=&rgba[(size_t)y*w*4];raw.insert(raw.end(),row,row+(size_t)w*4);}
    std::vector<BYTE> zlib; zlib.push_back(0x78);zlib.push_back(0x01); size_t pos=0;
    while(pos<raw.size()){size_t n=std::min((size_t)65535,raw.size()-pos);bool final=pos+n==raw.size();zlib.push_back(final?1:0);AppendLE16(zlib,(unsigned int)n);AppendLE16(zlib,(unsigned int)(~(unsigned int)n));zlib.insert(zlib.end(),raw.begin()+pos,raw.begin()+pos+n);pos+=n;}
    AppendBE32(zlib,Adler32PNG(raw.data(),raw.size()));
    png.clear(); const BYTE sig[8]={137,80,78,71,13,10,26,10};png.insert(png.end(),sig,sig+8);
    std::vector<BYTE> ihdr;AppendBE32(ihdr,(unsigned long)w);AppendBE32(ihdr,(unsigned long)h);ihdr.push_back(8);ihdr.push_back(6);ihdr.push_back(0);ihdr.push_back(0);ihdr.push_back(0);PNGChunk(png,"IHDR",ihdr);PNGChunk(png,"IDAT",zlib);PNGChunk(png,"IEND",std::vector<BYTE>());return true;
}
static BYTE QuantizarGIF(BYTE r,BYTE g,BYTE b) { unsigned int q=((unsigned int)(r>>5)<<5)|((unsigned int)(g>>5)<<2)|(unsigned int)(b>>6); return (BYTE)(1+(q*254u+127u)/255u); }
static void CriarPaletaGIF(std::vector<BYTE>& pal) {
    pal.assign(256*3,0); pal[0]=255;pal[1]=0;pal[2]=255;
    for(unsigned int i=1;i<256;++i){unsigned int q=((i-1)*255u+127u)/254u; unsigned int r=(q>>5)&7,g=(q>>2)&7,b=q&3;pal[i*3]=(BYTE)((r*255+3)/7);pal[i*3+1]=(BYTE)((g*255+3)/7);pal[i*3+2]=(BYTE)((b*255+1)/3);}
}
static bool EscreverGIFSubBlocks(HANDLE f,const std::vector<BYTE>& data) {
    size_t pos=0;while(pos<data.size()){BYTE n=(BYTE)std::min((size_t)255,data.size()-pos);if(!EscreverByteHandle(f,n)||!EscreverHandle(f,&data[pos],n))return false;pos+=n;}return EscreverByteHandle(f,0);
}
static bool CodificarLZWGIF(const std::vector<BYTE>& pixels,std::vector<BYTE>& out) {
    if(pixels.empty())return false; out.clear(); unsigned long bitBuf=0;int bitCount=0;int codeSize=9;unsigned int nextCode=258;
    auto putCode=[&](unsigned int code)->void{bitBuf|=(code<<bitCount);bitCount+=codeSize;while(bitCount>=8){out.push_back((BYTE)(bitBuf&255));bitBuf>>=8;bitCount-=8;}};
    std::unordered_map<unsigned int,unsigned short> dict;dict.reserve(4096);putCode(256);unsigned int prefix=pixels[0];
    for(size_t i=1;i<pixels.size();++i){unsigned int k=pixels[i];unsigned int key=(prefix<<8)|k;auto it=dict.find(key);if(it!=dict.end()){prefix=it->second;continue;}putCode(prefix);if(nextCode<4096){dict[key]=(unsigned short)nextCode++;if(nextCode>(1u<<codeSize)&&codeSize<12)++codeSize;}else{putCode(256);dict.clear();codeSize=9;nextCode=258;}prefix=k;}
    putCode(prefix);putCode(257);if(bitCount>0)out.push_back((BYTE)(bitBuf&255));return true;
}
static bool RenderizarGIFArquivo(HWND owner,const wchar_t* path,bool transparent) {
    double start=0,finish=0;size_t total=0;int fps=24;if(!ObterFaixaExportacao(start,finish,total,fps))return false;
    IniciarProgressoRender(owner,total,L"GIF animado"); HANDLE f=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL); if(f==INVALID_HANDLE_VALUE){FinalizarProgressoRender(false);MessageBoxW(owner,L"Não foi possível criar o arquivo GIF no diretório escolhido.",L"Kodanima",MB_OK|MB_ICONERROR);return false;}
    bool ok=true;const char sig[]="GIF89a";ok=EscreverHandle(f,sig,6);std::vector<BYTE> hdr;AppendLE16(hdr,(unsigned int)configAtual.largura);AppendLE16(hdr,(unsigned int)configAtual.altura);hdr.push_back(0xF7);hdr.push_back(0);hdr.push_back(0);std::vector<BYTE> palette;CriarPaletaGIF(palette);ok=ok&&EscreverHandle(f,hdr.data(),hdr.size())&&EscreverHandle(f,palette.data(),palette.size());
    const BYTE loopExt[]={0x21,0xFF,0x0B,'N','E','T','S','C','A','P','E','2','.','0',0x03,0x01,0x00,0x00,0x00};ok=ok&&EscreverHandle(f,loopExt,sizeof(loopExt));
    for(size_t i=0;ok&&i<total;++i){HBITMAP bm=ComporQuadroNoTempo(start+(double)i/fps);if(!bm){ok=false;break;}int w=0,h=0;std::vector<BYTE> rgba;ok=ObterPixelsRGBA(bm,w,h,rgba,transparent);DeleteObject(bm);if(!ok||w>65535||h>65535){ok=false;break;}
        std::vector<BYTE> indices((size_t)w*h);for(size_t p=0;p<indices.size();++p){const BYTE* px=&rgba[p*4];indices[p]=(transparent&&px[3]==0)?0:QuantizarGIF(px[0],px[1],px[2]);}
        std::vector<BYTE> gce={0x21,0xF9,0x04,(BYTE)(transparent?0x01:0x00)};AppendLE16(gce,(unsigned int)std::max(1,(int)std::lround(100.0/fps)));gce.push_back(0);gce.push_back(0);
        std::vector<BYTE> image;image.push_back(0x2C);AppendLE16(image,0);AppendLE16(image,0);AppendLE16(image,(unsigned int)w);AppendLE16(image,(unsigned int)h);image.push_back(0);
        std::vector<BYTE> compressed;ok=CodificarLZWGIF(indices,compressed);if(ok)ok=EscreverHandle(f,gce.data(),gce.size())&&EscreverHandle(f,image.data(),image.size())&&EscreverByteHandle(f,8)&&EscreverGIFSubBlocks(f,compressed);
        AtualizarProgressoRender(i+1,total);
    }
    BYTE trailer=0x3B;if(ok)ok=EscreverByteHandle(f,trailer);CloseHandle(f);
    if(!ok){DeleteFileW(path);FinalizarProgressoRender(false);MessageBoxW(owner,L"Falha ao renderizar GIF. Verifique as dimensões do quadro e o espaço disponível em disco.",L"Kodanima",MB_OK|MB_ICONERROR);return false;}
    FinalizarProgressoRender(true);return true;
}
static void AppendBE64Vector(std::vector<BYTE>& v,unsigned long long x){AppendBE32(v,(unsigned long)(x>>32));AppendBE32(v,(unsigned long)x);}
static std::vector<BYTE> AtomPayloadFullBox(){return std::vector<BYTE>(4,0);}
struct KodaMOVAudioData {
    std::vector<BYTE> pcm;
    DWORD sampleRate = 44100;
    DWORD channels = 2;
    unsigned long long sampleFrames = 0;
    bool hasAudio = false;
};

// Decodifica a faixa para PCM 16-bit intercalado. O MOV usa áudio QuickTime "sowt"
// (PCM little-endian), evitando uma dependência de codec de áudio adicional no mux.
// A saída cobre a duração inteira do vídeo; silêncio antes/depois do clipe mantém a sincronização.
static HRESULT KodaMOVDecodificarAudioPCM(double exportStart, double exportFinish, KodaMOVAudioData& out) {
    out = KodaMOVAudioData{};
    if (g_audioPath.empty() || exportFinish <= exportStart) return E_INVALIDARG;
    const double overlapStart = std::max(exportStart, g_audioStartSeconds);
    double overlapEnd = exportFinish;
    if (g_audioDurationSeconds > 0.0)
        overlapEnd = std::min(overlapEnd, g_audioStartSeconds + g_audioDurationSeconds);
    if (overlapEnd <= overlapStart) return S_OK; // não há interseção entre áudio e região exportada

    HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    const bool comInitialized = SUCCEEDED(hrCo);
    if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE) return hrCo;
    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    const bool mfStarted = SUCCEEDED(hr);
    if (FAILED(hr)) { if (comInitialized) CoUninitialize(); return hr; }

    IMFSourceReader* reader = NULL;
    IMFMediaType* nativeType = NULL;
    IMFMediaType* pcmType = NULL;
    UINT32 nativeChannels = 2, nativeRate = 44100;
    do {
        hr = MFCreateSourceReaderFromURL(g_audioPath.c_str(), NULL, &reader);
        if (FAILED(hr)) break;
        hr = reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
        if (SUCCEEDED(hr)) hr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        if (SUCCEEDED(hr)) hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &nativeType);
        if (FAILED(hr)) break;
        if (nativeType) {
            nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &nativeChannels);
            nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &nativeRate);
        }
        out.channels = (nativeChannels >= 1 && nativeChannels <= 2) ? nativeChannels : 2;
        out.sampleRate = (nativeRate >= 8000 && nativeRate <= 48000) ? nativeRate : 44100;
        hr = MFCreateMediaType(&pcmType);
        if (SUCCEEDED(hr)) hr = pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr)) hr = pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (SUCCEEDED(hr)) hr = pcmType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, out.channels);
        if (SUCCEEDED(hr)) hr = pcmType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, out.sampleRate);
        if (SUCCEEDED(hr)) hr = pcmType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (SUCCEEDED(hr)) hr = pcmType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, out.channels * 2);
        if (SUCCEEDED(hr)) hr = pcmType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, out.sampleRate * out.channels * 2);
        if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pcmType);
        if (FAILED(hr)) break;

        const double durationSeconds = std::max(0.0, exportFinish - exportStart);
        out.sampleFrames = (unsigned long long)std::llround(durationSeconds * (double)out.sampleRate);
        const unsigned long long blockAlign = (unsigned long long)out.channels * 2ULL;
        const unsigned long long byteCount = out.sampleFrames * blockAlign;
        // Limite defensivo de 256 MiB para não esgotar a RAM em MOV PCM sem compressão.
        if (!out.sampleFrames || byteCount > 256ULL * 1024ULL * 1024ULL || out.sampleFrames > 0xffffffffULL) {
            hr = E_OUTOFMEMORY; break;
        }
        out.pcm.assign((size_t)byteCount, 0);

        const double sourceStartSeconds = std::max(0.0, overlapStart - g_audioStartSeconds);
        PROPVARIANT position; PropVariantInit(&position); position.vt = VT_I8;
        position.hVal.QuadPart = (LONGLONG)std::llround(sourceStartSeconds * 10000000.0);
        hr = reader->SetCurrentPosition(GUID_NULL, position);
        PropVariantClear(&position);
        if (FAILED(hr)) break;

        bool copiedAny = false;
        bool done = false;
        while (SUCCEEDED(hr) && !done) {
            DWORD streamIndex = MF_SOURCE_READER_FIRST_AUDIO_STREAM, flags = 0;
            LONGLONG sourceTime = 0;
            IMFSample* sample = NULL;
            hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &sourceTime, &sample);
            if (FAILED(hr)) { if (sample) sample->Release(); break; }
            if (!sample) {
                if (flags & MF_SOURCE_READERF_ENDOFSTREAM) done = true;
                continue;
            }
            LONGLONG sampleTime = 0;
            HRESULT sampleHr = sample->GetSampleTime(&sampleTime);
            IMFMediaBuffer* buffer = NULL;
            BYTE* src = NULL; DWORD maxLength = 0, currentLength = 0; bool locked = false;
            if (SUCCEEDED(sampleHr)) sampleHr = sample->ConvertToContiguousBuffer(&buffer);
            if (SUCCEEDED(sampleHr)) { sampleHr = buffer->Lock(&src, &maxLength, &currentLength); locked = SUCCEEDED(sampleHr); }
            if (FAILED(sampleHr)) {
                if (locked && buffer) buffer->Unlock();
                if (buffer) buffer->Release(); sample->Release(); hr = sampleHr; break;
            }
            const LONGLONG absoluteTimelineTicks = (LONGLONG)std::llround(g_audioStartSeconds * 10000000.0) + sampleTime;
            const LONGLONG outputTicks = absoluteTimelineTicks - (LONGLONG)std::llround(exportStart * 10000000.0);
            long long dstStart = (long long)std::llround((double)outputTicks * (double)out.sampleRate / 10000000.0);
            const size_t sourceFrames = currentLength / (size_t)blockAlign;
            size_t srcSkip = 0;
            if (dstStart < 0) { srcSkip = (size_t)std::min<unsigned long long>((unsigned long long)sourceFrames, (unsigned long long)(-dstStart)); dstStart = 0; }
            if ((unsigned long long)dstStart >= out.sampleFrames) {
                done = true;
            } else if (sourceFrames > srcSkip) {
                const unsigned long long available = out.sampleFrames - (unsigned long long)dstStart;
                const size_t copyFrames = (size_t)std::min<unsigned long long>((unsigned long long)(sourceFrames - srcSkip), available);
                if (copyFrames > 0) {
                    BYTE* dest = out.pcm.data() + (size_t)dstStart * (size_t)blockAlign;
                    const BYTE* input = src + srcSkip * (size_t)blockAlign;
                    memcpy(dest, input, copyFrames * (size_t)blockAlign);
                    copiedAny = true;
                }
            }
            if (locked && buffer) buffer->Unlock();
            if (buffer) buffer->Release(); sample->Release();
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) done = true;
        }
        if (SUCCEEDED(hr) && !copiedAny) hr = MF_E_INVALIDMEDIATYPE;
        if (SUCCEEDED(hr)) out.hasAudio = true;
    } while (false);

    if (nativeType) nativeType->Release();
    if (pcmType) pcmType->Release();
    if (reader) reader->Release();
    if (mfStarted) MFShutdown();
    if (comInitialized) CoUninitialize();
    if (FAILED(hr)) out = KodaMOVAudioData{};
    return hr;
}

static std::vector<BYTE> ConstruirMoovMOV(int width, int height, int fps, size_t frameCount,
        const std::vector<unsigned int>& sampleSizes, const std::vector<unsigned long long>& offsets,
        const KodaMOVAudioData* audio = NULL, unsigned long long audioOffset = 0) {
    const bool hasAudio = audio && audio->hasAudio && !audio->pcm.empty();
    const unsigned long long durationMovie = (unsigned long long)std::llround((double)frameCount * 1000.0 / fps);
    const unsigned long movieDuration32 = (unsigned long)std::min(0xffffffffULL, durationMovie);
    std::vector<BYTE> mvhd(4,0);
    AppendBE32(mvhd,0); AppendBE32(mvhd,0); AppendBE32(mvhd,1000); AppendBE32(mvhd,movieDuration32);
    AppendBE32(mvhd,0x00010000); AppendBE16(mvhd,0x0100); AppendBE16(mvhd,0); AppendBE32(mvhd,0); AppendBE32(mvhd,0);
    const unsigned long matrix[9]={0x00010000,0,0,0,0x00010000,0,0,0,0x40000000};
    for(int i=0;i<9;++i) AppendBE32(mvhd,matrix[i]); for(int i=0;i<6;++i) AppendBE32(mvhd,0);
    AppendBE32(mvhd,hasAudio?3:2);

    std::vector<BYTE> tkhd; tkhd.insert(tkhd.end(),{0,0,0,7}); AppendBE32(tkhd,0); AppendBE32(tkhd,0); AppendBE32(tkhd,1); AppendBE32(tkhd,0); AppendBE32(tkhd,movieDuration32);
    AppendBE32(tkhd,0); AppendBE32(tkhd,0); AppendBE16(tkhd,0); AppendBE16(tkhd,0); AppendBE16(tkhd,0); AppendBE16(tkhd,0);
    for(int i=0;i<9;++i) AppendBE32(tkhd,matrix[i]); AppendBE32(tkhd,(unsigned long)width<<16); AppendBE32(tkhd,(unsigned long)height<<16);
    std::vector<BYTE> mdhd(4,0); AppendBE32(mdhd,0); AppendBE32(mdhd,0); AppendBE32(mdhd,(unsigned long)fps); AppendBE32(mdhd,(unsigned long)std::min<unsigned long long>(0xffffffffULL,(unsigned long long)frameCount)); AppendBE16(mdhd,0x55c4); AppendBE16(mdhd,0);
    std::vector<BYTE> hdlr(4,0); hdlr.insert(hdlr.end(),{'m','h','l','r','v','i','d','e','a','p','p','l'}); AppendBE32(hdlr,0); AppendBE32(hdlr,0); const char handlerName[]="Kodanima Video"; hdlr.insert(hdlr.end(),handlerName,handlerName+sizeof(handlerName));
    std::vector<BYTE> vmhd(4,0); AppendBE16(vmhd,0); AppendBE16(vmhd,0); AppendBE16(vmhd,0); AppendBE16(vmhd,0);
    std::vector<BYTE> url(4,0); url[3]=1; std::vector<BYTE> dref(4,0); AppendBE32(dref,1); AdicionarAtomMov(dref,"url ",url); std::vector<BYTE> dinf; AdicionarAtomMov(dinf,"dref",dref);
    std::vector<BYTE> visual; visual.insert(visual.end(),6,0); AppendBE16(visual,1); AppendBE16(visual,0); AppendBE16(visual,0); AppendBE32(visual,0); AppendBE32(visual,0); AppendBE32(visual,0);
    AppendBE16(visual,(unsigned int)width); AppendBE16(visual,(unsigned int)height); AppendBE32(visual,0x00480000); AppendBE32(visual,0x00480000); AppendBE32(visual,0); AppendBE16(visual,1); visual.push_back(8);
    const char cname[]="Kodanima PNG"; visual.push_back((BYTE)(sizeof(cname)-1)); visual.insert(visual.end(),cname,cname+sizeof(cname)-1); visual.resize(visual.size()+31-(sizeof(cname)-1),0); AppendBE16(visual,32); AppendBE16(visual,0xffff);
    std::vector<BYTE> sampleEntry=CriarAtomMov("png ",visual); std::vector<BYTE> stsd(4,0); AppendBE32(stsd,1); stsd.insert(stsd.end(),sampleEntry.begin(),sampleEntry.end());
    std::vector<BYTE> stts(4,0); AppendBE32(stts,1); AppendBE32(stts,(unsigned long)frameCount); AppendBE32(stts,1);
    std::vector<BYTE> stsc(4,0); AppendBE32(stsc,1); AppendBE32(stsc,1); AppendBE32(stsc,1); AppendBE32(stsc,1);
    std::vector<BYTE> stsz(4,0); AppendBE32(stsz,0); AppendBE32(stsz,(unsigned long)sampleSizes.size()); for(unsigned int sz:sampleSizes) AppendBE32(stsz,sz);
    std::vector<BYTE> co64(4,0); AppendBE32(co64,(unsigned long)offsets.size()); for(unsigned long long off:offsets) AppendBE64Vector(co64,off);
    std::vector<BYTE> stbl; AdicionarAtomMov(stbl,"stsd",stsd); AdicionarAtomMov(stbl,"stts",stts); AdicionarAtomMov(stbl,"stsc",stsc); AdicionarAtomMov(stbl,"stsz",stsz); AdicionarAtomMov(stbl,"co64",co64);
    std::vector<BYTE> minf; AdicionarAtomMov(minf,"vmhd",vmhd); AdicionarAtomMov(minf,"dinf",dinf); AdicionarAtomMov(minf,"stbl",stbl);
    std::vector<BYTE> mdia; AdicionarAtomMov(mdia,"mdhd",mdhd); AdicionarAtomMov(mdia,"hdlr",hdlr); AdicionarAtomMov(mdia,"minf",minf);
    std::vector<BYTE> trak; AdicionarAtomMov(trak,"tkhd",tkhd); AdicionarAtomMov(trak,"mdia",mdia);

    std::vector<BYTE> moov; AdicionarAtomMov(moov,"mvhd",mvhd); AdicionarAtomMov(moov,"trak",trak);
    if(hasAudio) {
        const unsigned long blockAlign=(unsigned long)audio->channels*2UL;
        const unsigned long frameCountAudio=(unsigned long)audio->sampleFrames;
        std::vector<BYTE> atkhd; atkhd.insert(atkhd.end(),{0,0,0,7}); AppendBE32(atkhd,0); AppendBE32(atkhd,0); AppendBE32(atkhd,2); AppendBE32(atkhd,0); AppendBE32(atkhd,movieDuration32);
        AppendBE32(atkhd,0); AppendBE32(atkhd,0); AppendBE16(atkhd,0); AppendBE16(atkhd,0); AppendBE16(atkhd,0x0100); AppendBE16(atkhd,0);
        for(int i=0;i<9;++i) AppendBE32(atkhd,matrix[i]); AppendBE32(atkhd,0); AppendBE32(atkhd,0);
        std::vector<BYTE> amdhd(4,0); AppendBE32(amdhd,0); AppendBE32(amdhd,0); AppendBE32(amdhd,(unsigned long)audio->sampleRate); AppendBE32(amdhd,frameCountAudio); AppendBE16(amdhd,0x55c4); AppendBE16(amdhd,0);
        std::vector<BYTE> ahdlr(4,0); ahdlr.insert(ahdlr.end(),{'m','h','l','r','s','o','u','n','a','p','p','l'}); AppendBE32(ahdlr,0); AppendBE32(ahdlr,0); const char audioName[]="Kodanima Audio"; ahdlr.insert(ahdlr.end(),audioName,audioName+sizeof(audioName));
        std::vector<BYTE> soundEntry; soundEntry.insert(soundEntry.end(),6,0); AppendBE16(soundEntry,1); AppendBE16(soundEntry,0); AppendBE16(soundEntry,0); AppendBE32(soundEntry,0);
        AppendBE16(soundEntry,(unsigned int)audio->channels); AppendBE16(soundEntry,16); AppendBE16(soundEntry,0); AppendBE16(soundEntry,0); AppendBE32(soundEntry,(unsigned long)audio->sampleRate*65536UL);
        std::vector<BYTE> asample=CriarAtomMov("sowt",soundEntry); std::vector<BYTE> astsd(4,0); AppendBE32(astsd,1); astsd.insert(astsd.end(),asample.begin(),asample.end());
        std::vector<BYTE> atts(4,0); AppendBE32(atts,1); AppendBE32(atts,frameCountAudio); AppendBE32(atts,1);
        std::vector<BYTE> atsc(4,0); AppendBE32(atsc,1); AppendBE32(atsc,1); AppendBE32(atsc,frameCountAudio); AppendBE32(atsc,1);
        std::vector<BYTE> atsz(4,0); AppendBE32(atsz,blockAlign); AppendBE32(atsz,frameCountAudio);
        std::vector<BYTE> aco64(4,0); AppendBE32(aco64,1); AppendBE64Vector(aco64,audioOffset);
        std::vector<BYTE> astbl; AdicionarAtomMov(astbl,"stsd",astsd); AdicionarAtomMov(astbl,"stts",atts); AdicionarAtomMov(astbl,"stsc",atsc); AdicionarAtomMov(astbl,"stsz",atsz); AdicionarAtomMov(astbl,"co64",aco64);
        std::vector<BYTE> smhd(4,0); AppendBE16(smhd,0); AppendBE16(smhd,0);
        std::vector<BYTE> aminf; AdicionarAtomMov(aminf,"smhd",smhd); AdicionarAtomMov(aminf,"dinf",dinf); AdicionarAtomMov(aminf,"stbl",astbl);
        std::vector<BYTE> amdia; AdicionarAtomMov(amdia,"mdhd",amdhd); AdicionarAtomMov(amdia,"hdlr",ahdlr); AdicionarAtomMov(amdia,"minf",aminf);
        std::vector<BYTE> atrak; AdicionarAtomMov(atrak,"tkhd",atkhd); AdicionarAtomMov(atrak,"mdia",amdia); AdicionarAtomMov(moov,"trak",atrak);
    }
    return CriarAtomMov("moov",moov);
}
static bool RenderizarMOVArquivo(HWND owner,const wchar_t* path,bool transparent,bool includeAudio) {
    double start=0,finish=0;size_t total=0;int fps=24;if(!ObterFaixaExportacao(start,finish,total,fps))return false;
    if((unsigned long long)configAtual.largura*(unsigned long long)configAtual.altura>40000000ULL){MessageBoxW(owner,L"A resolução é grande demais para a memória de um quadro PNG MOV. Reduza a resolução e tente novamente.",L"Kodanima MOV",MB_OK|MB_ICONWARNING);return false;}
    KodaMOVAudioData audio;
    if(includeAudio && !g_audioPath.empty()) {
        HRESULT audioHr=KodaMOVDecodificarAudioPCM(start,finish,audio);
        if(FAILED(audioHr)) {
            wchar_t msg[384]; swprintf(msg,384,L"Não foi possível decodificar/muxar o áudio para MOV (0x%08X). Desmarque 'Incluir áudio' ou use MP4 H.264.",(unsigned int)audioHr);
            MessageBoxW(owner,msg,L"Kodanima MOV — áudio",MB_OK|MB_ICONERROR); return false;
        }
    }
    IniciarProgressoRender(owner,total,L"MOV / PNG");HANDLE f=CreateFileW(path,GENERIC_WRITE|GENERIC_READ,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE){FinalizarProgressoRender(false);MessageBoxW(owner,L"Não foi possível criar o arquivo MOV no diretório escolhido.",L"Kodanima",MB_OK|MB_ICONERROR);return false;}
    std::vector<BYTE> ftyp=CriarAtomMov("ftyp",std::vector<BYTE>{'q','t',' ', ' ',0,2,0,0,'q','t',' ',' '});bool ok=EscreverHandle(f,ftyp.data(),ftyp.size());BYTE mdatHeader[16]={0,0,0,1,'m','d','a','t',0,0,0,0,0,0,0,0};ok=ok&&EscreverHandle(f,mdatHeader,sizeof(mdatHeader));
    std::vector<unsigned long long> offsets;std::vector<unsigned int> sizes;offsets.reserve(total);sizes.reserve(total);unsigned long long payloadBytes=0;
    for(size_t i=0;ok&&i<total;++i){HBITMAP bm=ComporQuadroNoTempo(start+(double)i/fps);if(!bm){ok=false;break;}int w=0,h=0;std::vector<BYTE> rgba;ok=ObterPixelsRGBA(bm,w,h,rgba,transparent);DeleteObject(bm);std::vector<BYTE> png;if(ok)ok=CriarPNG_RGBA(w,h,rgba,png);if(!ok||png.size()>0xffffffffULL){ok=false;break;}unsigned long long offset=0;if(!PosicaoHandle(f,offset)){ok=false;break;}offsets.push_back(offset);sizes.push_back((unsigned int)png.size());payloadBytes+=png.size();ok=EscreverHandle(f,png.data(),png.size());AtualizarProgressoRender(i+1,total);}
    unsigned long long audioOffset=0;
    if(ok && audio.hasAudio && !audio.pcm.empty()) {
        if(!PosicaoHandle(f,audioOffset)) ok=false;
        else { ok=EscreverHandle(f,audio.pcm.data(),audio.pcm.size()); if(ok) payloadBytes+=(unsigned long long)audio.pcm.size(); }
    }
    if(ok){unsigned long long endPos=0;ok=PosicaoHandle(f,endPos);if(ok){LARGE_INTEGER at{};at.QuadPart=28;ok=SetFilePointerEx(f,at,NULL,FILE_BEGIN);unsigned long long mdatSize=16+payloadBytes;BYTE sz[8];for(int i=0;i<8;++i)sz[i]=(BYTE)(mdatSize>>(56-i*8));if(ok)ok=EscreverHandle(f,sz,8);LARGE_INTEGER end{};end.QuadPart=(LONGLONG)endPos;if(!SetFilePointerEx(f,end,NULL,FILE_BEGIN))ok=false;}}
    if(ok){std::vector<BYTE> moov=ConstruirMoovMOV(configAtual.largura,configAtual.altura,fps,total,sizes,offsets,audio.hasAudio?&audio:NULL,audioOffset);ok=EscreverHandle(f,moov.data(),moov.size());}CloseHandle(f);
    if(!ok){DeleteFileW(path);FinalizarProgressoRender(false);MessageBoxW(owner,L"Falha ao renderizar MOV. O arquivo usa vídeo PNG e, se ativado, áudio PCM sem compressão; verifique memória disponível e espaço em disco.",L"Kodanima MOV",MB_OK|MB_ICONERROR);return false;}
    FinalizarProgressoRender(true);return true;
}
struct KodaMP4AudioContext {
    IMFSourceReader* reader = NULL;
    DWORD streamIndex = 0;
    IMFSample* pending = NULL;
    bool endOfStream = false;
    LONGLONG clipOffset = 0; // timeline clip start relative to export start, in 100-ns units
    LONGLONG cutoff = 0;
};
static void KodaMP4AudioDestroy(KodaMP4AudioContext& a) {
    if(a.pending){a.pending->Release();a.pending=NULL;}
    if(a.reader){a.reader->Release();a.reader=NULL;}
}
static HRESULT KodaMP4AudioInit(KodaMP4AudioContext& a, IMFSinkWriter* writer,
                                DWORD* streamIndex, double exportStart, double exportFinish) {
    if(g_audioPath.empty() || !writer || !streamIndex) return E_INVALIDARG;
    HRESULT hr=MFCreateSourceReaderFromURL(g_audioPath.c_str(),NULL,&a.reader);
    if(FAILED(hr)) return hr;
    a.reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE);
    hr=a.reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE);
    IMFMediaType* nativeType=NULL; IMFMediaType* pcmType=NULL; IMFMediaType* aacType=NULL;
    UINT32 channels=2, sampleRate=44100;
    if(SUCCEEDED(hr)) hr=a.reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,&nativeType);
    if(SUCCEEDED(hr) && nativeType) { nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,&channels); nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,&sampleRate); }
    if(channels<1 || channels>8) channels=2;
    if(sampleRate<8000 || sampleRate>192000) sampleRate=44100;
    if(SUCCEEDED(hr)) hr=MFCreateMediaType(&pcmType);
    if(SUCCEEDED(hr)) hr=pcmType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
    if(SUCCEEDED(hr)) hr=pcmType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM);
    if(SUCCEEDED(hr)) hr=pcmType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,channels);
    if(SUCCEEDED(hr)) hr=pcmType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,sampleRate);
    if(SUCCEEDED(hr)) hr=pcmType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
    if(SUCCEEDED(hr)) hr=pcmType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,channels*2);
    if(SUCCEEDED(hr)) hr=pcmType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,sampleRate*channels*2);
    if(SUCCEEDED(hr)) hr=a.reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,NULL,pcmType);
    if(SUCCEEDED(hr)) hr=MFCreateMediaType(&aacType);
    if(SUCCEEDED(hr)) hr=aacType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
    if(SUCCEEDED(hr)) hr=aacType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AAC);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,channels);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,sampleRate);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,24000);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,1);
    if(SUCCEEDED(hr)) hr=aacType->SetUINT32(MF_MT_AVG_BITRATE,192000);
    if(SUCCEEDED(hr)) hr=writer->AddStream(aacType,&a.streamIndex);
    if(SUCCEEDED(hr)) hr=writer->SetInputMediaType(a.streamIndex,pcmType,NULL);
    a.clipOffset=(LONGLONG)std::llround((g_audioStartSeconds-exportStart)*10000000.0);
    double renderDuration=std::max(0.0,exportFinish-exportStart);
    if(g_audioDurationSeconds>0.0) renderDuration=std::min(renderDuration,std::max(0.0,g_audioStartSeconds+g_audioDurationSeconds-exportStart));
    a.cutoff=(LONGLONG)std::llround(renderDuration*10000000.0);
    if(nativeType) nativeType->Release(); if(pcmType) pcmType->Release(); if(aacType) aacType->Release();
    if(FAILED(hr)) KodaMP4AudioDestroy(a);
    return hr;
}
static HRESULT KodaMP4AudioPump(KodaMP4AudioContext& a, IMFSinkWriter* writer, LONGLONG beforeTime) {
    if(!a.reader || !writer || a.endOfStream) return S_OK;
    beforeTime=std::min(beforeTime,a.cutoff);
    for(;;) {
        if(!a.pending) {
            DWORD flags=0, actualStream=MF_SOURCE_READER_FIRST_AUDIO_STREAM; LONGLONG readerTime=0;
            HRESULT hr=a.reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,&actualStream,&flags,&readerTime,&a.pending);
            if(FAILED(hr)) return hr;
            if(flags & MF_SOURCE_READERF_ENDOFSTREAM) a.endOfStream=true;
            if(!a.pending) { if(a.endOfStream) return S_OK; else continue; }
        }
        LONGLONG sourceTime=0;
        HRESULT hr=a.pending->GetSampleTime(&sourceTime);
        if(FAILED(hr)) sourceTime=0;
        const LONGLONG outputTime=sourceTime+a.clipOffset;
        if(outputTime>=beforeTime || outputTime>=a.cutoff) return S_OK;
        IMFSample* send=a.pending; a.pending=NULL;
        if(outputTime>=0) {
            hr=send->SetSampleTime(outputTime);
            if(SUCCEEDED(hr)) hr=writer->WriteSample(a.streamIndex,send);
            send->Release();
            if(FAILED(hr)) return hr;
        } else send->Release(); // áudio anterior ao início da faixa exportada
    }
}

static bool RenderizarMP4Arquivo(HWND hwndOwner,const wchar_t* arquivo,bool incluirAudio) {
    double inicio=0,fim=0;size_t totalAmostrasExportacao=0;int fps=24;
    if(!ObterFaixaExportacao(inicio,fim,totalAmostrasExportacao,fps)){MessageBoxW(hwndOwner,L"Não há quadros para renderizar.",L"Kodanima",MB_OK|MB_ICONWARNING);return false;}
    IniciarProgressoRender(hwndOwner,totalAmostrasExportacao,L"MP4 H.264");
    HRESULT hrCo=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);bool comInicializado=SUCCEEDED(hrCo);
    HRESULT hr=MFStartup(MF_VERSION,MFSTARTUP_FULL);
    if(FAILED(hr)){if(comInicializado)CoUninitialize();FinalizarProgressoRender(false);MessageBoxW(hwndOwner,L"Não foi possível iniciar o Media Foundation. O Media Foundation/H.264 não existe no Windows XP. No XP, use GIF ou MOV/PNG, ou exporte MP4 em outro sistema com codificador H.264.",L"Kodanima - MP4",MB_OK|MB_ICONERROR);return false;}
    IMFMediaType* tipoSaida=NULL;IMFMediaType* tipoEntrada=NULL;IMFSinkWriter* gravador=NULL;DWORD indiceFluxo=0;
    KodaMP4AudioContext audioCtx{}; bool audioConfigurado=false;
    hr=MFCreateSinkWriterFromURL(arquivo,NULL,NULL,&gravador);
    if(SUCCEEDED(hr))hr=MFCreateMediaType(&tipoSaida);if(SUCCEEDED(hr))hr=tipoSaida->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);if(SUCCEEDED(hr))hr=tipoSaida->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264);if(SUCCEEDED(hr))hr=tipoSaida->SetUINT32(MF_MT_AVG_BITRATE,8000000);if(SUCCEEDED(hr))hr=tipoSaida->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);if(SUCCEEDED(hr))hr=MFSetAttributeSize(tipoSaida,MF_MT_FRAME_SIZE,configAtual.largura,configAtual.altura);if(SUCCEEDED(hr))hr=MFSetAttributeRatio(tipoSaida,MF_MT_FRAME_RATE,fps,1);if(SUCCEEDED(hr))hr=MFSetAttributeRatio(tipoSaida,MF_MT_PIXEL_ASPECT_RATIO,1,1);if(SUCCEEDED(hr))hr=gravador->AddStream(tipoSaida,&indiceFluxo);
    if(SUCCEEDED(hr))hr=MFCreateMediaType(&tipoEntrada);if(SUCCEEDED(hr))hr=tipoEntrada->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);if(SUCCEEDED(hr))hr=tipoEntrada->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);if(SUCCEEDED(hr))hr=tipoEntrada->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);if(SUCCEEDED(hr))hr=MFSetAttributeSize(tipoEntrada,MF_MT_FRAME_SIZE,configAtual.largura,configAtual.altura);if(SUCCEEDED(hr))hr=MFSetAttributeRatio(tipoEntrada,MF_MT_FRAME_RATE,fps,1);if(SUCCEEDED(hr))hr=MFSetAttributeRatio(tipoEntrada,MF_MT_PIXEL_ASPECT_RATIO,1,1);if(SUCCEEDED(hr))hr=gravador->SetInputMediaType(indiceFluxo,tipoEntrada,NULL);
    if(SUCCEEDED(hr) && incluirAudio && !g_audioPath.empty()) { hr=KodaMP4AudioInit(audioCtx,gravador,&audioCtx.streamIndex,inicio,fim); audioConfigurado=SUCCEEDED(hr); }
    if(SUCCEEDED(hr))hr=gravador->BeginWriting();
    const LONGLONG duracaoAmostra=10000000LL/fps;LONGLONG tempo=0;
    for(size_t i=0;SUCCEEDED(hr)&&i<totalAmostrasExportacao;++i){HBITMAP bitmap=ComporQuadroNoTempo(inicio+(double)i/fps);if(!bitmap){hr=E_FAIL;break;}BITMAP bm{};GetObjectW(bitmap,sizeof(bm),&bm);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=bm.bmWidth;bi.bmiHeader.biHeight=-bm.bmHeight;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;DWORD bytes=(DWORD)bm.bmWidth*(DWORD)bm.bmHeight*4;std::vector<BYTE> pixels(bytes);HDC dc=GetDC(NULL);int lines=GetDIBits(dc,bitmap,0,bm.bmHeight,pixels.data(),&bi,DIB_RGB_COLORS);ReleaseDC(NULL,dc);if(lines==0){DeleteObject(bitmap);hr=E_FAIL;break;}for(int y=0;y<bm.bmHeight/2;++y){BYTE* top=&pixels[(size_t)y*bm.bmWidth*4];BYTE* bottom=&pixels[(size_t)(bm.bmHeight-1-y)*bm.bmWidth*4];for(int x=0;x<bm.bmWidth*4;++x){BYTE t=top[x];top[x]=bottom[x];bottom[x]=t;}}
        IMFMediaBuffer* buffer=NULL;IMFSample* sample=NULL;BYTE* dest=NULL;DWORD maxLen=0,curLen=0;hr=MFCreateMemoryBuffer(bytes,&buffer);if(SUCCEEDED(hr))hr=buffer->Lock(&dest,&maxLen,&curLen);if(SUCCEEDED(hr)){CopyMemory(dest,pixels.data(),bytes);buffer->Unlock();hr=buffer->SetCurrentLength(bytes);}if(SUCCEEDED(hr))hr=MFCreateSample(&sample);if(SUCCEEDED(hr))hr=sample->AddBuffer(buffer);if(SUCCEEDED(hr))hr=sample->SetSampleTime(tempo);if(SUCCEEDED(hr))hr=sample->SetSampleDuration(duracaoAmostra);if(SUCCEEDED(hr))hr=gravador->WriteSample(indiceFluxo,sample);if(SUCCEEDED(hr)&&audioConfigurado)hr=KodaMP4AudioPump(audioCtx,gravador,tempo+duracaoAmostra);tempo+=duracaoAmostra;if(sample)sample->Release();if(buffer)buffer->Release();DeleteObject(bitmap);AtualizarProgressoRender(i+1,totalAmostrasExportacao);}
    if(SUCCEEDED(hr)&&audioConfigurado)hr=KodaMP4AudioPump(audioCtx,gravador,audioCtx.cutoff);
    KodaMP4AudioDestroy(audioCtx);
    if(SUCCEEDED(hr)&&gravador)hr=gravador->Finalize();if(gravador)gravador->Release();if(tipoEntrada)tipoEntrada->Release();if(tipoSaida)tipoSaida->Release();MFShutdown();if(comInicializado)CoUninitialize();
    if(FAILED(hr)){DeleteFileW(arquivo);FinalizarProgressoRender(false);wchar_t msg[256];wsprintfW(msg,L"Falha ao renderizar MP4 (código 0x%08X). Verifique suporte ao codec H.264.",(unsigned int)hr);MessageBoxW(hwndOwner,msg,L"Kodanima - MP4",MB_OK|MB_ICONERROR);return false;}
    FinalizarProgressoRender(true);return true;
}
static void AtualizarVisibilidadeTransparencia(HWND h) {
    int sel=(int)SendMessageW(GetDlgItem(h,ID_RENDER_FORMAT),CB_GETCURSEL,0,0);int fmt=(int)SendMessageW(GetDlgItem(h,ID_RENDER_FORMAT),CB_GETITEMDATA,sel,0);
    ShowWindow(GetDlgItem(h,ID_RENDER_ALPHA),(fmt==2||fmt==3)?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(h,ID_RENDER_AUDIO),(fmt==1||fmt==3)?SW_SHOW:SW_HIDE);
    InvalidateRect(h,NULL,TRUE);
}
static LRESULT CALLBACK WndProcRenderOptions(HWND h,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_COMMAND) {
        int id=LOWORD(w);
        if(id==ID_RENDER_FORMAT&&HIWORD(w)==CBN_SELCHANGE){AtualizarVisibilidadeTransparencia(h);return 0;}
        if(id==ID_RENDER_BROWSE){BROWSEINFOW bi{};bi.hwndOwner=h;const std::wstring browseTitle=KodaTraduzirTexto(L"Escolha a pasta onde salvar a animação"); bi.lpszTitle=browseTitle.c_str();bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;LPITEMIDLIST pid=SHBrowseForFolderW(&bi);if(pid){wchar_t path[MAX_PATH]={0};if(SHGetPathFromIDListW(pid,path))SetWindowTextW(GetDlgItem(h,ID_RENDER_FOLDER),path);CoTaskMemFree(pid);}return 0;}
        if(id==ID_RENDER_CANCEL){DestroyWindow(h);return 0;}
        if(id==ID_RENDER_START){
            if(listaFrames.empty()){MessageBoxW(h,L"Não há quadros para renderizar.",L"Kodanima",MB_OK|MB_ICONWARNING);return 0;}
            int sel=(int)SendMessageW(GetDlgItem(h,ID_RENDER_FORMAT),CB_GETCURSEL,0,0);int fmt=(int)SendMessageW(GetDlgItem(h,ID_RENDER_FORMAT),CB_GETITEMDATA,sel,0);bool alpha=SendMessageW(GetDlgItem(h,ID_RENDER_ALPHA),BM_GETCHECK,0,0)==BST_CHECKED;bool includeAudio=SendMessageW(GetDlgItem(h,ID_RENDER_AUDIO),BM_GETCHECK,0,0)==BST_CHECKED;
            wchar_t folder[MAX_PATH]={0},name[256]={0};GetWindowTextW(GetDlgItem(h,ID_RENDER_FOLDER),folder,MAX_PATH);GetWindowTextW(GetDlgItem(h,ID_RENDER_NAME),name,256);
            std::wstring dir=folder,stem=name;
            while(!stem.empty()&&(stem.front()==L' '||stem.front()==L'\t'))stem.erase(stem.begin());
            while(!stem.empty()&&(stem.back()==L' '||stem.back()==L'\t'||stem.back()==L'.'))stem.pop_back();
            for(size_t ni=0;ni<stem.size();++ni)if(stem[ni]==L'\\'||stem[ni]==L'/'||stem[ni]==L':'||stem[ni]==L'*'||stem[ni]==L'?'||stem[ni]==L'"'||stem[ni]==L'<'||stem[ni]==L'>'||stem[ni]==L'|')stem[ni]=L'_';
            if(dir.empty()||stem.empty()){MessageBoxW(h,L"Informe o diretório e o nome do arquivo.",L"Kodanima",MB_OK|MB_ICONWARNING);return 0;}
            DWORD attr=GetFileAttributesW(dir.c_str());
            if(attr!=INVALID_FILE_ATTRIBUTES && !(attr&FILE_ATTRIBUTE_DIRECTORY)){MessageBoxW(h,L"O caminho selecionado existe, mas não é uma pasta.",L"Kodanima",MB_OK|MB_ICONERROR);return 0;}
            if(attr==INVALID_FILE_ATTRIBUTES){int ce=SHCreateDirectoryExW(h,dir.c_str(),NULL);if(ce!=ERROR_SUCCESS&&ce!=ERROR_ALREADY_EXISTS&&ce!=ERROR_FILE_EXISTS){MessageBoxW(h,L"Não consegui criar o diretório de saída.",L"Kodanima",MB_OK|MB_ICONERROR);return 0;}}
            const wchar_t* ext=fmt==1?L".mp4":fmt==2?L".gif":L".mov";if(stem.size()>=4&&_wcsicmp(stem.c_str()+stem.size()-4,ext)==0)stem.resize(stem.size()-4);std::wstring output=dir;if(!output.empty()&&output.back()!=L'\\'&&output.back()!=L'/')output+=L"\\";output+=stem;output+=ext;
            if(GetFileAttributesW(output.c_str())!=INVALID_FILE_ATTRIBUTES){if(MessageBoxW(h,L"Esse arquivo já existe. Deseja substituí-lo?",L"Kodanima — Renderizar",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;}
            if((fmt==1||fmt==3) && includeAudio && g_audioPath.empty()){MessageBoxW(h,L"Nenhum áudio foi importado. O vídeo será renderizado mudo; importe uma faixa na timeline ou desmarque a opção.",L"Kodanima — Áudio",MB_OK|MB_ICONINFORMATION);}
            EnableWindow(h,FALSE);bool ok=fmt==1?RenderizarMP4Arquivo(h,output.c_str(),includeAudio):(fmt==2?RenderizarGIFArquivo(h,output.c_str(),alpha):RenderizarMOVArquivo(h,output.c_str(),alpha,includeAudio));EnableWindow(h,TRUE);SetForegroundWindow(h);
            if(ok){std::wstring status=KodaTraduzirTexto(L"Concluído: ")+output;SetWindowTextW(GetDlgItem(h,ID_RENDER_STATUS),status.c_str());}else SetWindowTextW(GetDlgItem(h,ID_RENDER_STATUS),L"A renderização falhou. Confira a mensagem de erro.");return 0;
        }
    }
    if(msg==WM_CLOSE){DestroyWindow(h);return 0;}
    if(msg==WM_DESTROY){if(g_hwndRenderOptions==h)g_hwndRenderOptions=NULL;return 0;}
    return DefWindowProcW(h,msg,w,l);
}
bool RenderizarMP4(HWND hwndOwner) {
    if(g_hwndRenderOptions&&IsWindow(g_hwndRenderOptions)){ShowWindow(g_hwndRenderOptions,SW_SHOWNORMAL);SetForegroundWindow(g_hwndRenderOptions);return true;}
    static bool registered=false;if(!registered){WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProcRenderOptions;wc.hInstance=hInstGlobal;wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"KodanimaRenderOptionsClass";RegisterClassExW(&wc);registered=true;}
    RECT wr{};if(hwndOwner&&IsWindow(hwndOwner))GetWindowRect(hwndOwner,&wr);else SetRect(&wr,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));int x=wr.left+(wr.right-wr.left-520)/2,y=wr.top+(wr.bottom-wr.top-350)/2;x=std::max(0,std::min(GetSystemMetrics(SM_CXSCREEN)-520,x));y=std::max(0,std::min(GetSystemMetrics(SM_CYSCREEN)-350,y));
    g_hwndRenderOptions=CreateWindowExW(WS_EX_TOOLWINDOW,L"KodanimaRenderOptionsClass",L"Kodanima — Renderizar animação",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,x,y,520,350,hwndOwner,NULL,hInstGlobal,NULL);if(!g_hwndRenderOptions)return false;HWND h=g_hwndRenderOptions;
    CreateWindowW(L"STATIC",L"Formato de arquivo:",WS_VISIBLE|WS_CHILD,18,18,150,20,h,NULL,hInstGlobal,NULL);HWND combo=CreateWindowW(L"COMBOBOX",L"",WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST,175,15,305,150,h,(HMENU)ID_RENDER_FORMAT,hInstGlobal,NULL);SendMessageW(combo,CB_ADDSTRING,0,(LPARAM)L"MP4 — H.264 (sem transparência)");SendMessageW(combo,CB_SETITEMDATA,0,1);SendMessageW(combo,CB_ADDSTRING,0,(LPARAM)L"GIF animado");SendMessageW(combo,CB_SETITEMDATA,1,2);SendMessageW(combo,CB_ADDSTRING,0,(LPARAM)L"MOV — vídeo PNG (com transparência)");SendMessageW(combo,CB_SETITEMDATA,2,3);SendMessageW(combo,CB_SETCURSEL,0,0);
    CreateWindowW(L"STATIC",L"Nome do arquivo:",WS_VISIBLE|WS_CHILD,18,57,150,22,h,NULL,hInstGlobal,NULL);CreateWindowW(L"EDIT",L"animacao",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,175,54,305,25,h,(HMENU)ID_RENDER_NAME,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Diretório de saída:",WS_VISIBLE|WS_CHILD,18,94,150,22,h,NULL,hInstGlobal,NULL);CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,175,91,225,25,h,(HMENU)ID_RENDER_FOLDER,hInstGlobal,NULL);CreateWindowW(L"BUTTON",L"Procurar...",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|WS_TABSTOP,405,91,75,25,h,(HMENU)ID_RENDER_BROWSE,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Fundo transparente (GIF/MOV)",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|WS_TABSTOP,175,128,305,24,h,(HMENU)ID_RENDER_ALPHA,hInstGlobal,NULL);
    HWND audioCheck=CreateWindowW(L"BUTTON",L"Incluir áudio (MP4/MOV)",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|WS_TABSTOP,175,153,305,24,h,(HMENU)ID_RENDER_AUDIO,hInstGlobal,NULL);
    SendMessageW(audioCheck,BM_SETCHECK,BST_CHECKED,0);
    CreateWindowW(L"STATIC",L"MP4/H.264 exige Windows 7+; MOV/PNG pode incluir áudio PCM sem compressão.",WS_VISIBLE|WS_CHILD,18,181,465,34,h,NULL,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"",WS_VISIBLE|WS_CHILD,18,213,465,32,h,(HMENU)ID_RENDER_STATUS,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Renderizar",WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|WS_TABSTOP,278,252,115,32,h,(HMENU)ID_RENDER_START,hInstGlobal,NULL);CreateWindowW(L"BUTTON",L"Fechar",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|WS_TABSTOP,405,252,75,32,h,(HMENU)ID_RENDER_CANCEL,hInstGlobal,NULL);
    wchar_t defaultDir[MAX_PATH]={0};if(FAILED(SHGetFolderPathW(NULL,CSIDL_MYVIDEO|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,defaultDir)))SHGetFolderPathW(NULL,CSIDL_PERSONAL|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,defaultDir);SetWindowTextW(GetDlgItem(h,ID_RENDER_FOLDER),defaultDir);SendMessageW(GetDlgItem(h,ID_RENDER_ALPHA),BM_SETCHECK,BST_UNCHECKED,0);AtualizarVisibilidadeTransparencia(h);ShowWindow(h,SW_SHOWNORMAL);UpdateWindow(h);return true;
}

void IrParaQuadro(int indice) {
    if (listaFrames.empty()) return;
    if (indice < 0) indice = 0;
    if (indice >= (int)listaFrames.size()) indice = (int)listaFrames.size() - 1;
    frameAtual = indice;
    if (frameAtual >= 0 && frameAtual < (int)listaFrames.size()) durationUnit = (listaFrames[frameAtual].unidadeDuracao == 0 ? 0 : 1);
    if (hwndTimeline) {
        HWND edit = GetDlgItem(hwndTimeline, ID_EDIT_FRAME_SECONDS);
        if (edit) AtualizarCampoDuracao(hwndTimeline);
        InvalidateRect(hwndTimeline, NULL, TRUE);
    }
    InvalidarCacheComposicaoCanvas(); if (hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
}

// Cria uma cópia tingida para Onion Skin. A cor de fundo permanece intacta
// para que TransparentBlt a remova e só as linhas de referência apareçam.
HBITMAP CriarBitmapOnion(HBITMAP origem, COLORREF corTint) {
    if (!origem) return NULL;
    BITMAP bm{}; GetObject(origem, sizeof(bm), &bm);
    if (bm.bmWidth <= 0 || bm.bmHeight <= 0) return NULL;
    BITMAPINFO bi{}; bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth; bi.bmiHeader.biHeight = -bm.bmHeight;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    std::vector<BYTE> px((size_t)bm.bmWidth * bm.bmHeight * 4);
    HDC dc = GetDC(NULL); int got = GetDIBits(dc, origem, 0, bm.bmHeight, px.data(), &bi, DIB_RGB_COLORS); ReleaseDC(NULL, dc);
    if (!got) return NULL;
    COLORREF bg = configAtual.corFundo;
    int br = GetRValue(bg), bgc = GetGValue(bg), bb = GetBValue(bg);
    int tr = GetRValue(corTint), tg = GetGValue(corTint), tb = GetBValue(corTint);
    for (size_t i=0; i<px.size(); i+=4) {
        int b=px[i], g=px[i+1], r=px[i+2];
        // Pixels do fundo ficam exatamente com a cor-chave; linhas recebem a cor de referência.
        int lum=(r+g+b)/3;
        if (abs(r-br)<8 && abs(g-bgc)<8 && abs(b-bb)<8) { px[i]=(BYTE)bb; px[i+1]=(BYTE)bgc; px[i+2]=(BYTE)br; }
        else {
            int intensidade=(255-lum)*opacidadeOnion/100;
            px[i]=(BYTE)((bb*(255-intensidade/2)+tb*(intensidade/2))/255);
            px[i+1]=(BYTE)((bgc*(255-intensidade/2)+tg*(intensidade/2))/255);
            px[i+2]=(BYTE)((br*(255-intensidade/2)+tr*(intensidade/2))/255);
        }
        px[i+3]=0;
    }
    HDC dc2=GetDC(NULL); HBITMAP out=CreateDIBitmap(dc2, &bi.bmiHeader, CBM_INIT, px.data(), &bi, DIB_RGB_COLORS); ReleaseDC(NULL,dc2);
    return out;
}

// Cria um quadro composto: os traços do quadro atual permanecem no topo;
// apenas os pixels do fundo atual são substituídos por referências onion tintadas.
HBITMAP CriarBitmapComOnionAbaixo(HBITMAP atual, HBITMAP anterior, HBITMAP proximo) {
    if (!atual) return NULL;
    BITMAP bm{}; if (!GetObjectW(atual, sizeof(bm), &bm) || bm.bmWidth<=0 || bm.bmHeight<=0) return NULL;
    const int w=bm.bmWidth, h=bm.bmHeight;
    BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=w;
    bi.bmiHeader.biHeight=-h; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
    const size_t bytes=(size_t)w*(size_t)h*4;
    std::vector<BYTE> cur(bytes), prev(bytes), next(bytes);
    HDC dc=GetDC(NULL); if(!dc) return NULL;
    int got=GetDIBits(dc,atual,0,h,cur.data(),&bi,DIB_RGB_COLORS);
    if(got && anterior) GetDIBits(dc,anterior,0,h,prev.data(),&bi,DIB_RGB_COLORS);
    if(got && proximo) GetDIBits(dc,proximo,0,h,next.data(),&bi,DIB_RGB_COLORS);
    ReleaseDC(NULL,dc); if(!got) return NULL;
    const COLORREF bg=configAtual.corFundo;
    const int br=GetRValue(bg), bgc=GetGValue(bg), bb=GetBValue(bg);
    const int opacity=std::max(0,std::min(100,opacidadeOnion));
    auto isBg=[&](const std::vector<BYTE>& p,size_t i)->bool {
        return std::abs((int)p[i+2]-br)<10 && std::abs((int)p[i+1]-bgc)<10 && std::abs((int)p[i]-bb)<10;
    };
    for(size_t i=0;i<bytes;i+=4) {
        if(!isBg(cur,i)) continue; // nunca pinta por cima do desenho atual
        int outR=br, outG=bgc, outB=bb, count=0;
        if(anterior && !isBg(prev,i)) {
            int r=GetRValue(RGB(255,0,0)), g=0, b=0;
            outR=(outR*(100-opacity)+r*opacity)/100;
            outG=(outG*(100-opacity)+g*opacity)/100;
            outB=(outB*(100-opacity)+b*opacity)/100; ++count;
        }
        if(proximo && !isBg(next,i)) {
            int r=0, g=GetGValue(RGB(0,180,0)), b=0;
            if(count) { outR=(outR+r)/2; outG=(outG+g)/2; outB=(outB+b)/2; }
            else { outR=(outR*(100-opacity)+r*opacity)/100; outG=(outG*(100-opacity)+g*opacity)/100; outB=(outB*(100-opacity)+b*opacity)/100; }
        }
        cur[i]=(BYTE)outB; cur[i+1]=(BYTE)outG; cur[i+2]=(BYTE)outR; cur[i+3]=0;
    }
    dc=GetDC(NULL); if(!dc) return NULL;
    HBITMAP out=CreateDIBitmap(dc,&bi.bmiHeader,CBM_INIT,cur.data(),&bi,DIB_RGB_COLORS);
    ReleaseDC(NULL,dc); return out;
}

void DesenharBitmapCanvas(HDC hdc, HBITMAP bitmap, HWND hwnd, bool onion, COLORREF corTint) {
    if (!bitmap) return;
    BITMAP bmp{}; GetObject(bitmap,sizeof(bmp),&bmp);
    RECT rc; GetClientRect(hwnd,&rc);
    int dw=std::max(1,(int)(bmp.bmWidth*zoomFactor));
    int dh=std::max(1,(int)(bmp.bmHeight*zoomFactor));
    int cx=rc.right/2+panX, cy=rc.bottom/2+panY;
    double a=(double)anguloCanvas*3.14159265358979323846/180.0, ca=cos(a), sa=sin(a);
    double hw=dw/2.0, hh=dh/2.0;
    POINT pts[3];
    pts[0].x=(LONG)(cx-hw*ca+hh*sa); pts[0].y=(LONG)(cy-hw*sa-hh*ca);
    pts[1].x=(LONG)(cx+hw*ca+hh*sa); pts[1].y=(LONG)(cy+hw*sa-hh*ca);
    pts[2].x=(LONG)(cx-hw*ca-hh*sa); pts[2].y=(LONG)(cy-hw*sa+hh*ca);
    HBITMAP drawBmp=bitmap;
    if (onion) { drawBmp=CriarBitmapOnion(bitmap,corTint); if (!drawBmp) return; }
    HDC mem=CreateCompatibleDC(hdc); HBITMAP old=(HBITMAP)SelectObject(mem,drawBmp);
    SetStretchBltMode(hdc,COLORONCOLOR);
    const bool semRotacao = (std::fabs(anguloCanvas) < 0.001);
    if (semRotacao) {
        // O caso comum (sem rotação) usa StretchBlt/TransparentBlt, mais rápido
        // para zoom e pan que o PlgBlt de três pontos.
        if (onion) TransparentBlt(hdc,pts[0].x,pts[0].y,dw,dh,mem,0,0,bmp.bmWidth,bmp.bmHeight,configAtual.corFundo);
        else StretchBlt(hdc,pts[0].x,pts[0].y,dw,dh,mem,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
    } else if (onion) {
        TransparentBlt(hdc, std::min(pts[0].x,std::min(pts[1].x,pts[2].x)), std::min(pts[0].y,std::min(pts[1].y,pts[2].y)),
            std::max(1,dw), std::max(1,dh), mem,0,0,bmp.bmWidth,bmp.bmHeight,configAtual.corFundo);
    } else {
        PlgBlt(hdc,pts,mem,0,0,bmp.bmWidth,bmp.bmHeight,NULL,0,0);
    }
    SelectObject(mem,old); DeleteDC(mem); if (onion) DeleteObject(drawBmp);
}

void FecharTodasJanelas() {
    // Fecha apenas as janelas de edição. A tela inicial permanece aberta.
    DestruirBackbufferCanvas();
    if (g_canvasCompositeCache) DeleteObject(g_canvasCompositeCache);
    g_canvasCompositeCache = NULL;
    g_canvasCompositeCacheFrame = -1;
    g_canvasCompositeCacheDirty = true;
    if (previewAtivo && hwndCanvas) { KillTimer(hwndCanvas, ID_TIMER_PREVIEW); previewAtivo=false; }
    KodaAudioParar();
    HWND c=hwndCanvas, t=hwndTimeline, tools=hwndTools, shortcuts=hwndShortcutPanel;
    hwndCanvas=hwndTimeline=hwndTools=hwndShortcutPanel=NULL;
    if (shortcuts && IsWindow(shortcuts)) DestroyWindow(shortcuts);
    if (c && IsWindow(c)) DestroyWindow(c);
    if (t && IsWindow(t)) DestroyWindow(t);
    if (tools && IsWindow(tools)) DestroyWindow(tools);
    if (hwndMain && IsWindow(hwndMain)) { ShowWindow(hwndMain, SW_SHOW); SetForegroundWindow(hwndMain); }
}

// Reprodução eficiente: MCI usa os decodificadores e a pilha de áudio do Windows,
// evitando decodificar o arquivo inteiro em RAM. Se um codec (por exemplo Opus em
// Windows antigo) não estiver instalado, a importação continua visível mas a reprodução falha.
static bool KodaAudioEnsureMCI() {
    if(!g_mciModule) g_mciModule=LoadLibraryW(L"winmm.dll");
    if(g_mciModule && !g_mciSendStringW) g_mciSendStringW=(KodaMciSendStringWProc)GetProcAddress(g_mciModule,"mciSendStringW");
    return g_mciSendStringW!=NULL;
}
static UINT KodaMci(const std::wstring& command, LPWSTR ret=NULL, UINT retLen=0) {
    if(!KodaAudioEnsureMCI()) return 1;
    return g_mciSendStringW(command.c_str(),ret,retLen,NULL);
}
static bool KodaAudioTryOpenWavDirectSound() {
    if(g_audioPath.size()<4) return false;
    std::wstring ext=g_audioPath.substr(g_audioPath.find_last_of(L'.')==std::wstring::npos?g_audioPath.size():g_audioPath.find_last_of(L'.'));
    for(size_t i=0;i<ext.size();++i) ext[i]=(wchar_t)towlower(ext[i]);
    if(ext!=L".wav") return false;
    HANDLE file=CreateFileW(g_audioPath.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE) return false;
    DWORD size=GetFileSize(file,NULL);
    if(size<44 || size>64u*1024u*1024u) { CloseHandle(file); return false; }
    std::vector<BYTE> bytes(size); DWORD got=0; BOOL readOk=ReadFile(file,bytes.data(),size,&got,NULL); CloseHandle(file);
    if(!readOk || got!=size || memcmp(bytes.data(),"RIFF",4)!=0 || memcmp(bytes.data()+8,"WAVE",4)!=0) return false;
    WAVEFORMATEX fmt{}; DWORD dataOffset=0,dataBytes=0; bool haveFmt=false;
    size_t pos=12;
    while(pos+8<=bytes.size()) {
        DWORD chunkSize=0; memcpy(&chunkSize,&bytes[pos+4],4); size_t body=pos+8;
        if(body+chunkSize>bytes.size()) break;
        if(memcmp(&bytes[pos],"fmt ",4)==0 && chunkSize>=16) {
            memcpy(&fmt,&bytes[body],std::min<size_t>(chunkSize,sizeof(WAVEFORMATEX))); haveFmt=true;
        } else if(memcmp(&bytes[pos],"data",4)==0) { dataOffset=(DWORD)body; dataBytes=chunkSize; }
        pos=body+chunkSize+(chunkSize&1u);
    }
    if(!haveFmt || !dataOffset || !dataBytes || fmt.wFormatTag!=WAVE_FORMAT_PCM || fmt.nChannels<1 || fmt.nChannels>2 || fmt.wBitsPerSample!=16 || !fmt.nBlockAlign || !fmt.nSamplesPerSec || dataOffset+dataBytes>bytes.size()) return false;
    if(!g_directSoundModule) g_directSoundModule=LoadLibraryW(L"dsound.dll");
    if(g_directSoundModule && !g_directSoundCreate8) g_directSoundCreate8=(KodaDirectSoundCreate8Proc)GetProcAddress(g_directSoundModule,"DirectSoundCreate8");
    if(!g_directSoundCreate8) return false;
    if(g_directSoundDevice) { g_directSoundDevice->Release(); g_directSoundDevice=NULL; }
    if(FAILED(g_directSoundCreate8(NULL,&g_directSoundDevice,NULL)) || !g_directSoundDevice) return false;
    HWND target=(hwndTimeline&&IsWindow(hwndTimeline))?hwndTimeline:hwndMain;
    if(!target || FAILED(g_directSoundDevice->SetCooperativeLevel(target,DSSCL_NORMAL))) { g_directSoundDevice->Release(); g_directSoundDevice=NULL; return false; }
    g_directSoundWaveFormat=fmt; g_directSoundDataBytes=dataBytes;
    DSBUFFERDESC desc{}; desc.dwSize=sizeof(desc); desc.dwFlags=DSBCAPS_CTRLVOLUME|DSBCAPS_GLOBALFOCUS|DSBCAPS_GETCURRENTPOSITION2; desc.dwBufferBytes=dataBytes; desc.dwReserved=0; desc.lpwfxFormat=&g_directSoundWaveFormat;
    if(FAILED(g_directSoundDevice->CreateSoundBuffer(&desc,&g_directSoundBuffer,NULL)) || !g_directSoundBuffer) { g_directSoundDevice->Release(); g_directSoundDevice=NULL; return false; }
    void* p1=NULL; void* p2=NULL; DWORD b1=0,b2=0;
    HRESULT hr=g_directSoundBuffer->Lock(0,dataBytes,&p1,&b1,&p2,&b2,0);
    if(FAILED(hr)) { g_directSoundBuffer->Release(); g_directSoundBuffer=NULL; g_directSoundDevice->Release(); g_directSoundDevice=NULL; return false; }
    if(p1 && b1) memcpy(p1,&bytes[dataOffset],b1); if(p2 && b2) memcpy(p2,&bytes[dataOffset+b1],b2);
    g_directSoundBuffer->Unlock(p1,b1,p2,b2);
    g_audioDurationSeconds=(double)dataBytes/(double)fmt.nAvgBytesPerSec;
    g_audioBackendDirectSound=true; g_audioDeviceOpen=true; g_audioPlayable=true; return true;
}
static void KodaAudioParar() {
    if(hwndTimeline && IsWindow(hwndTimeline)) KillTimer(hwndTimeline,ID_TIMER_AUDIO_SCRUB_STOP);
    if(g_audioBackendDirectSound) { if(g_directSoundBuffer) g_directSoundBuffer->Stop(); return; }
    if(!g_audioDeviceOpen || !KodaAudioEnsureMCI()) return;
    KodaMci(L"stop kodanimaAudio");
}
static void KodaAudioFechar() {
    if(g_audioBackendDirectSound) {
        if(g_directSoundBuffer){g_directSoundBuffer->Stop();g_directSoundBuffer->Release();g_directSoundBuffer=NULL;}
        if(g_directSoundDevice){g_directSoundDevice->Release();g_directSoundDevice=NULL;}
        g_audioBackendDirectSound=false;
    } else if(g_audioDeviceOpen && KodaAudioEnsureMCI()) { KodaMci(L"stop kodanimaAudio"); KodaMci(L"close kodanimaAudio"); }
    g_audioDeviceOpen=false; g_audioPlayable=false; g_directSoundDataBytes=0;
}
static bool KodaAudioAbrir(HWND owner, bool avisar) {
    if(g_audioPath.empty()) return false;
    if(g_audioDeviceOpen && g_audioPlayable) return true;
    KodaAudioFechar();
    if(KodaAudioTryOpenWavDirectSound()) return true;
    if(!KodaAudioEnsureMCI()) return false;
    std::wstring cmd=L"open \""+g_audioPath+L"\" alias kodanimaAudio";
    UINT err=KodaMci(cmd);
    if(err) {
        // Força o driver adequado caso a associação automática não reconheça a extensão.
        std::wstring low=g_audioPath; for(size_t i=0;i<low.size();++i) low[i]=(wchar_t)towlower(low[i]);
        const wchar_t* type=(low.size()>=4 && low.substr(low.size()-4)==L".wav")?L"waveaudio":L"mpegvideo";
        cmd=L"open \""+g_audioPath+L"\" type "+type+L" alias kodanimaAudio"; err=KodaMci(cmd);
    }
    if(err) {
        if(avisar) MessageBoxW(owner,L"O arquivo foi adicionado à faixa, mas o Windows não encontrou um decodificador MCI para reproduzi-lo. MP3/WAV costumam funcionar; Opus depende de suporte/códec instalado.",L"Áudio do Kodanima",MB_OK|MB_ICONWARNING);
        g_audioDeviceOpen=false; g_audioPlayable=false; return false;
    }
    g_audioDeviceOpen=true; g_audioPlayable=true;
    KodaMci(L"set kodanimaAudio time format milliseconds");
    wchar_t length[64]={0};
    if(KodaMci(L"status kodanimaAudio length",length,63)==0) {
        double ms=wcstod(length,NULL); if(ms>0.0) g_audioDurationSeconds=ms/1000.0;
    }
    return true;
}
static void KodaAudioTocarDaTimeline(double segundos) {
    if(!KodaAudioAbrir(hwndTimeline,false)) return;
    KodaAudioParar();
    if(segundos<g_audioStartSeconds) return;
    double offset=segundos-g_audioStartSeconds;
    if(g_audioDurationSeconds>0.0 && offset>=g_audioDurationSeconds) return;
    if(g_audioBackendDirectSound && g_directSoundBuffer) {
        DWORD bytePos=(DWORD)std::min<double>((double)g_directSoundDataBytes,std::max(0.0,offset*g_directSoundWaveFormat.nAvgBytesPerSec));
        if(g_directSoundWaveFormat.nBlockAlign) bytePos-=bytePos%g_directSoundWaveFormat.nBlockAlign;
        g_directSoundBuffer->SetCurrentPosition(bytePos); g_directSoundBuffer->Play(0,0,0); return;
    }
    unsigned long long from=(unsigned long long)std::max(0.0,offset*1000.0);
    wchar_t cmd[256]; swprintf(cmd,256,L"play kodanimaAudio from %llu",from); KodaMci(cmd);
}
static void KodaAudioScrub(double segundos) {
    if(!g_audioScrubEnabled || g_audioPath.empty()) return;
    DWORD now=GetTickCount(); if(now-g_audioLastScrubTick<65) return; g_audioLastScrubTick=now;
    if(!KodaAudioAbrir(hwndTimeline,false)) return;
    KodaAudioParar();
    if(segundos<g_audioStartSeconds) return;
    double offset=segundos-g_audioStartSeconds;
    if(g_audioDurationSeconds>0.0 && offset>=g_audioDurationSeconds) return;
    if(g_audioBackendDirectSound && g_directSoundBuffer) {
        DWORD bytePos=(DWORD)std::min<double>((double)g_directSoundDataBytes,std::max(0.0,offset*g_directSoundWaveFormat.nAvgBytesPerSec));
        if(g_directSoundWaveFormat.nBlockAlign) bytePos-=bytePos%g_directSoundWaveFormat.nBlockAlign;
        g_directSoundBuffer->SetCurrentPosition(bytePos); g_directSoundBuffer->Play(0,0,0);
        if(hwndTimeline) SetTimer(hwndTimeline,ID_TIMER_AUDIO_SCRUB_STOP,140,NULL); return;
    }
    unsigned long long from=(unsigned long long)std::max(0.0,offset*1000.0);
    unsigned long long to=from+140;
    if(g_audioDurationSeconds>0.0) to=std::min(to,(unsigned long long)(g_audioDurationSeconds*1000.0));
    if(to<=from) return;
    wchar_t cmd[256]; swprintf(cmd,256,L"play kodanimaAudio from %llu to %llu",from,to); KodaMci(cmd);
}

// Importação unificada de mídia. O GDI+ é carregado dinamicamente para manter
// a compatibilidade do build com Windows XP sem exigir gdiplus.lib.
static HBITMAP KodaCarregarImagemColorida(const wchar_t* path) {
    HMODULE mod=LoadLibraryW(L"gdiplus.dll");
    if(!mod) return NULL;
    typedef INT (WINAPI *StartupFn)(ULONG_PTR*,const void*,void*);
    typedef INT (WINAPI *CreateFn)(LPCWSTR,void**);
    typedef INT (WINAPI *ToBitmapFn)(void*,HBITMAP*,COLORREF);
    typedef INT (WINAPI *DisposeFn)(void*);
    typedef VOID (WINAPI *ShutdownFn)(ULONG_PTR);
    struct StartupInputLocal { UINT32 version; void* callback; BOOL suppressThread; BOOL suppressCodec; };
    StartupFn startup=(StartupFn)GetProcAddress(mod,"GdiplusStartup");
    CreateFn create=(CreateFn)GetProcAddress(mod,"GdipCreateBitmapFromFile");
    ToBitmapFn toBitmap=(ToBitmapFn)GetProcAddress(mod,"GdipCreateHBITMAPFromBitmap");
    DisposeFn dispose=(DisposeFn)GetProcAddress(mod,"GdipDisposeImage");
    ShutdownFn shutdown=(ShutdownFn)GetProcAddress(mod,"GdiplusShutdown");
    if(!startup||!create||!toBitmap||!dispose||!shutdown){FreeLibrary(mod);return NULL;}
    StartupInputLocal si{}; si.version=1; ULONG_PTR token=0;
    if(startup(&token,&si,NULL)!=0){FreeLibrary(mod);return NULL;}
    void* image=NULL; HBITMAP result=NULL;
    if(create(path,&image)==0 && image) {
        // GDI+ espera ARGB, não COLORREF, como cor de fundo de HBITMAP.
        if(toBitmap(image,&result,0xFFFFFFFFu)!=0) result=NULL;
        dispose(image);
    }
    shutdown(token); FreeLibrary(mod);
    return result;
}

static std::wstring KodaExtensaoArquivo(const wchar_t* path) {
    if(!path) return L"";
    std::wstring ext=path;
    size_t slash=ext.find_last_of(L"\\/");
    size_t dot=ext.find_last_of(L'.');
    if(dot==std::wstring::npos || (slash!=std::wstring::npos && dot<slash)) return L"";
    ext=ext.substr(dot);
    for(size_t i=0;i<ext.size();++i) ext[i]=(wchar_t)towlower(ext[i]);
    return ext;
}
static bool KodaEhExtensao(const std::wstring& ext, const wchar_t* listaSeparada) {
    std::wstring list=listaSeparada;
    size_t p=0;
    while(p<list.size()) {
        size_t e=list.find(L';',p); if(e==std::wstring::npos)e=list.size();
        if(list.substr(p,e-p)==ext) return true;
        p=e+1;
    }
    return false;
}
static std::wstring KodaNomeBase(const wchar_t* path) {
    std::wstring name=path?path:L""; size_t p=name.find_last_of(L"\\/");
    if(p!=std::wstring::npos) name=name.substr(p+1); return name;
}
static bool KodaObterPastaProjeto(std::wstring& root) {
    root.clear();
    if(caminhoProjetoAtivo[0] && GetFileAttributesW(caminhoProjetoAtivo)!=INVALID_FILE_ATTRIBUTES) {
        root=caminhoProjetoAtivo;
    } else {
        // Garante que o projeto novo tenha um project.json antes de criar assets.
        SalvarProjetoCompleto();
        if(caminhoProjetoAtivo[0] && GetFileAttributesW(caminhoProjetoAtivo)!=INVALID_FILE_ATTRIBUTES) root=caminhoProjetoAtivo;
    }
    if(root.empty()) return false;
    DWORD attr=GetFileAttributesW(root.c_str());
    if(attr==INVALID_FILE_ATTRIBUTES) {
        if(!CreateDirectoryW(root.c_str(),NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    } else if(!(attr&FILE_ATTRIBUTE_DIRECTORY)) return false;
    return true;
}
static bool KodaCriarArquivoEmAssets(const wchar_t* source, std::wstring& absolutePath, std::wstring& relativePath) {
    if(!source || !source[0]) return false;
    std::wstring root;
    if(!KodaObterPastaProjeto(root)) return false;
    std::wstring assets=root+L"\\assets";
    DWORD attr=GetFileAttributesW(assets.c_str());
    if(attr==INVALID_FILE_ATTRIBUTES && !CreateDirectoryW(assets.c_str(),NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    const std::wstring ext=KodaExtensaoArquivo(source);
    std::wstring original=KodaNomeBase(source);
    if(original.empty()) original=L"midia"+ext;
    std::wstring stem=original;
    size_t dot=stem.find_last_of(L'.'); if(dot!=std::wstring::npos) stem.resize(dot);
    // Mantém o nome original. Em caso de colisão, acrescenta um número sem sobrescrever arquivos.
    for(unsigned i=0;i<100000;++i) {
        std::wstring leaf=(i==0?original:stem+L" ("+std::to_wstring(i+1)+L")"+ext);
        std::wstring dst=assets+L"\\"+leaf;
        if(GetFileAttributesW(dst.c_str())!=INVALID_FILE_ATTRIBUTES) continue;
        if(CopyFileW(source,dst.c_str(),TRUE)) {
            absolutePath=dst; relativePath=L"assets/"; relativePath+=leaf; return true;
        }
        DWORD err=GetLastError();
        if(err==ERROR_FILE_EXISTS || err==ERROR_ALREADY_EXISTS) continue;
        return false;
    }
    return false;
}
static void KodaRegistrarAsset(const std::wstring& relativePath,const std::wstring& type,
                               double startSeconds,double durationSeconds,int frame,int layer,int x,int y) {
    KodaProjectMediaAsset a; a.relativePath=relativePath; a.name=KodaNomeBase(relativePath.c_str()); a.type=type;
    a.startSeconds=std::max(0.0,startSeconds); a.durationSeconds=std::max(0.0,durationSeconds);
    a.frame=std::max(0,frame); a.layer=std::max(0,layer); a.x=x; a.y=y;
    g_projectMediaAssets.push_back(a);
}
static double KodaDetectarDuracaoMCI(const std::wstring& path) {
    if(path.empty() || !KodaAudioEnsureMCI()) return 0.0;
    KodaMci(L"close kodanimaProbe");
    std::wstring cmd=L"open \""+path+L"\" alias kodanimaProbe";
    if(KodaMci(cmd)!=0) return 0.0;
    KodaMci(L"set kodanimaProbe time format milliseconds");
    wchar_t len[64]={0}; double seconds=0.0;
    if(KodaMci(L"status kodanimaProbe length",len,63)==0) seconds=wcstod(len,NULL)/1000.0;
    KodaMci(L"close kodanimaProbe");
    if(seconds<0.01 || seconds>86400.0) return 0.0;
    return seconds;
}
static HBITMAP KodaCopiarHBITMAP(HBITMAP source) {
    if(!source) return NULL;
    BITMAP bm{}; if(!GetObjectW(source,sizeof(bm),&bm) || bm.bmWidth<=0 || bm.bmHeight<=0) return NULL;
    HDC dc=GetDC(NULL); if(!dc) return NULL;
    HBITMAP copy=CreateCompatibleBitmap(dc,bm.bmWidth,bm.bmHeight);
    if(copy) {
        HDC src=CreateCompatibleDC(dc), dst=CreateCompatibleDC(dc);
        if(!src || !dst) { if(src)DeleteDC(src); if(dst)DeleteDC(dst); DeleteObject(copy); copy=NULL; }
        else {
            HGDIOBJ os=SelectObject(src,source), od=SelectObject(dst,copy);
            if(!BitBlt(dst,0,0,bm.bmWidth,bm.bmHeight,src,0,0,SRCCOPY)) { DeleteObject(copy); copy=NULL; }
            SelectObject(src,os); SelectObject(dst,od); DeleteDC(src); DeleteDC(dst);
        }
    }
    ReleaseDC(NULL,dc); return copy;
}
static HBITMAP KodaBitmapDIBClipboard(UINT format) {
    HANDLE h=GetClipboardData(format); if(!h) return NULL;
    SIZE_T total=GlobalSize(h); BYTE* bytes=(BYTE*)GlobalLock(h);
    if(!bytes || total<sizeof(BITMAPINFOHEADER)) { if(bytes)GlobalUnlock(h); return NULL; }
    BITMAPINFOHEADER* bih=(BITMAPINFOHEADER*)bytes;
    if(bih->biSize<sizeof(BITMAPINFOHEADER) || bih->biSize>total || bih->biWidth<=0 || bih->biHeight==0 ||
       bih->biWidth>50000 || bih->biHeight>50000 || bih->biHeight< -50000) { GlobalUnlock(h); return NULL; }
    size_t offset=bih->biSize;
    if(bih->biSize==sizeof(BITMAPINFOHEADER) && (bih->biCompression==BI_BITFIELDS || bih->biCompression==BI_ALPHABITFIELDS))
        offset+=(bih->biCompression==BI_ALPHABITFIELDS?4u:3u)*sizeof(DWORD);
    if(bih->biBitCount<=8) {
        DWORD colors=bih->biClrUsed?bih->biClrUsed:(1u<<bih->biBitCount);
        offset+=(size_t)colors*sizeof(RGBQUAD);
    } else if(bih->biClrUsed) offset+=(size_t)bih->biClrUsed*sizeof(RGBQUAD);
    HBITMAP out=NULL;
    if(offset<total) {
        HDC dc=GetDC(NULL);
        if(dc) { out=CreateDIBitmap(dc,bih,CBM_INIT,bytes+offset,(BITMAPINFO*)bih,DIB_RGB_COLORS); ReleaseDC(NULL,dc); }
    }
    GlobalUnlock(h); return out;
}
static HBITMAP KodaBitmapClipboardPNG() {
    UINT pngFormat=RegisterClipboardFormatW(L"PNG");
    if(!pngFormat) return NULL;
    HANDLE h=GetClipboardData(pngFormat); if(!h) return NULL;
    SIZE_T n=GlobalSize(h); BYTE* b=(BYTE*)GlobalLock(h);
    if(!b || n<8 || n>128u*1024u*1024u) { if(b)GlobalUnlock(h); return NULL; }
    wchar_t tempDir[MAX_PATH]={0},tempFile[MAX_PATH]={0};
    HBITMAP bitmap=NULL;
    if(GetTempPathW(MAX_PATH,tempDir) && GetTempFileNameW(tempDir,L"kdp",0,tempFile)) {
        HANDLE out=CreateFileW(tempFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
        if(out!=INVALID_HANDLE_VALUE) {
            DWORD written=0; BOOL ok=WriteFile(out,b,(DWORD)n,&written,NULL); CloseHandle(out);
            if(ok && written==n) bitmap=KodaCarregarImagemColorida(tempFile);
        }
        DeleteFileW(tempFile);
    }
    GlobalUnlock(h); return bitmap;
}
static HBITMAP KodaObterImagemClipboard() {
    if(!OpenClipboard(hwndCanvas?hwndCanvas:hwndTimeline)) return NULL;
    HBITMAP b=NULL;
#ifdef CF_DIBV5
    if(IsClipboardFormatAvailable(CF_DIBV5)) b=KodaBitmapDIBClipboard(CF_DIBV5);
#endif
    if(!b && IsClipboardFormatAvailable(CF_DIB)) b=KodaBitmapDIBClipboard(CF_DIB);
    if(!b && IsClipboardFormatAvailable(CF_BITMAP)) b=KodaCopiarHBITMAP((HBITMAP)GetClipboardData(CF_BITMAP));
    // Navegadores normalmente colocam PNG nativo no clipboard além de CF_DIB.
    if(!b && IsClipboardFormatAvailable(RegisterClipboardFormatW(L"PNG"))) b=KodaBitmapClipboardPNG();
    CloseClipboard(); return b;
}
static void KodaAtivarColagemImagem(HWND owner,HBITMAP imported,const std::wstring& relPath,bool registrarAsset) {
    if(!imported) return;
    BITMAP bm{};
    if(!GetObjectW(imported,sizeof(bm),&bm) || bm.bmWidth<1 || bm.bmHeight<1 ||
       bm.bmWidth>configAtual.largura*4 || bm.bmHeight>configAtual.altura*4) {
        DeleteObject(imported);
        MessageBoxW(owner,L"A imagem é inválida ou grande demais para o projeto (limite de 4x o canvas por eixo).",L"Importar imagem",MB_OK|MB_ICONWARNING);
        return;
    }
    if(bitmapClipboardSelecao) DeleteObject(bitmapClipboardSelecao);
    bitmapClipboardSelecao=imported;
    if(bitmapPreviewColagemVermelho){DeleteObject(bitmapPreviewColagemVermelho);bitmapPreviewColagemVermelho=NULL;}
    bitmapPreviewColagemVermelho=CriarPreviewVermelho(bitmapClipboardSelecao);
    colandoSelecao=true; colagemModoRotacao=false; colagemModoEscala=false;
    colagemEscala=1.0f; colagemAngulo=0.0; transformacaoSelecaoAtiva=false;
    POINT p{}; GetCursorPos(&p); if(hwndCanvas) ScreenToClient(hwndCanvas,&p);
    int x=0,y=0;
    if(hwndCanvas && ScreenToBitmapPoint(hwndCanvas,BitmapCamadaAtual(),p,x,y)) {
        cursorColagemBmp.x=x-bm.bmWidth/2; cursorColagemBmp.y=y-bm.bmHeight/2;
    } else { cursorColagemBmp.x=(configAtual.largura-bm.bmWidth)/2; cursorColagemBmp.y=(configAtual.altura-bm.bmHeight)/2; }
    if(registrarAsset && !relPath.empty()) {
        // Se uma nova imagem for importada antes de confirmar a prévia anterior,
        // remove o registro pendente antigo para não deixar um asset fantasma.
        if(!g_pendingImageAssetRelativePath.empty()) {
            for(size_t ai=0;ai<g_projectMediaAssets.size();) {
                if(g_projectMediaAssets[ai].type==L"image" && g_projectMediaAssets[ai].relativePath==g_pendingImageAssetRelativePath) g_projectMediaAssets.erase(g_projectMediaAssets.begin()+ai);
                else ++ai;
            }
        }
        KodaRegistrarAsset(relPath,L"image",timelinePlayheadSeconds,0.0,frameAtual,selectedTimelineLayer,cursorColagemBmp.x,cursorColagemBmp.y);
        g_pendingImageAssetRelativePath=relPath;
    } else g_pendingImageAssetRelativePath.clear();
    if(hwndCanvas){SetForegroundWindow(hwndCanvas);SetFocus(hwndCanvas);SetCapture(hwndCanvas);InvalidateRect(hwndCanvas,NULL,FALSE);}
    if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
    SalvarProjetoCompleto();
}
static void KodaImportarImagemNaCamada(HWND owner) { // compatibilidade para chamadas antigas
    KodaMostrarDialogoImportar(owner);
}
static int KodaContarAssetsTipo(const wchar_t* type) {
    int n=0; for(const auto& a:g_projectMediaAssets) if(a.type==type) ++n; return n;
}
static std::wstring KodaCaminhoAssetAbsoluto(const std::wstring& rel) {
    std::wstring r=rel; for(size_t i=0;i<r.size();++i) if(r[i]==L'/') r[i]=L'\\';
    return std::wstring(caminhoProjetoAtivo)+L"\\"+r;
}
static void KodaLimparAssetsExpirados() {
    DWORD now=GetTickCount();
    for(size_t i=0;i<g_deletedMediaAssets.size();) {
        if((DWORD)(now-g_deletedMediaAssets[i].deletedAt)>=30000u) {
            std::wstring abs=KodaCaminhoAssetAbsoluto(g_deletedMediaAssets[i].asset.relativePath);
            DeleteFileW(abs.c_str());
            g_deletedMediaAssets.erase(g_deletedMediaAssets.begin()+i);
        } else ++i;
    }
}
static bool KodaRestaurarUltimaExclusaoMedia() {
    KodaLimparAssetsExpirados();
    if(g_deletedMediaAssets.empty()) return false;
    KodaDeletedMediaAsset d=g_deletedMediaAssets.back(); g_deletedMediaAssets.pop_back();
    std::wstring abs=KodaCaminhoAssetAbsoluto(d.asset.relativePath);
    if(GetFileAttributesW(abs.c_str())==INVALID_FILE_ATTRIBUTES) return false;
    g_projectMediaAssets.push_back(d.asset);
    if(d.asset.type==L"audio") {
        KodaAudioFechar(); g_audioPath=abs; g_audioRelativePath=d.asset.relativePath;
        g_audioStartSeconds=d.asset.startSeconds; g_audioDurationSeconds=d.asset.durationSeconds;
        KodaAudioAbrir(hwndTimeline?hwndTimeline:hwndMain,false);
    }
    if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
    SalvarProjetoCompleto(); return true;
}
static bool KodaExcluirAssetSelecionado(HWND owner) {
    KodaLimparAssetsExpirados();
    int i=g_selectedMediaAssetIndex;
    if(i<0 || i>=(int)g_projectMediaAssets.size()) return false;
    KodaProjectMediaAsset a=g_projectMediaAssets[i];
    if(a.type!=L"audio" && a.type!=L"video" && a.type!=L"gif") return false;
    KodaDeletedMediaAsset d; d.asset=a; d.deletedAt=GetTickCount(); g_deletedMediaAssets.push_back(d);
    g_projectMediaAssets.erase(g_projectMediaAssets.begin()+i); g_selectedMediaAssetIndex=-1;
    if(a.type==L"audio" && g_audioRelativePath==a.relativePath) {
        KodaAudioFechar(); g_audioPath.clear(); g_audioRelativePath.clear(); g_audioStartSeconds=0.0; g_audioDurationSeconds=0.0;
        // Ativa a faixa de áudio mais recente que ainda exista, se houver.
        for(int j=(int)g_projectMediaAssets.size()-1;j>=0;--j) if(g_projectMediaAssets[j].type==L"audio") {
            g_audioRelativePath=g_projectMediaAssets[j].relativePath; g_audioPath=KodaCaminhoAssetAbsoluto(g_audioRelativePath);
            g_audioStartSeconds=g_projectMediaAssets[j].startSeconds; g_audioDurationSeconds=g_projectMediaAssets[j].durationSeconds;
            KodaAudioAbrir(owner,false); break;
        }
    }
    if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
    SalvarProjetoCompleto(); return true;
}

static void KodaImportarArquivo(HWND owner,const wchar_t* path) {
    if(!path || !path[0]) return;
    const std::wstring ext=KodaExtensaoArquivo(path);
    const bool audio=KodaEhExtensao(ext,L".mp3;.wav;.opus;.m4a;.aac;.flac;.ogg;.wma");
    const bool video=KodaEhExtensao(ext,L".mp4;.mov;.avi;.wmv;.mkv;.webm;.mpeg;.mpg;.m4v;.3gp");
    const bool gif=(ext==L".gif");
    const bool image=KodaEhExtensao(ext,L".bmp;.png;.jpg;.jpeg;.webp;.tif;.tiff");
    if(!audio && !video && !gif && !image) {
        MessageBoxW(owner,L"Formato não reconhecido. Escolha uma imagem BMP/PNG/JPG/WebP/TIFF/GIF, áudio ou vídeo compatível.",L"Importar",MB_OK|MB_ICONINFORMATION);
        return;
    }
    // Limites por projeto: até 10 faixas de áudio e 5 clipes de vídeo/GIF.
    if(audio && KodaContarAssetsTipo(L"audio")>=10) { MessageBoxW(owner,L"Este projeto já tem 10 camadas/faixas de música. Exclua uma antes de importar outra.",L"Limite de áudio",MB_OK|MB_ICONINFORMATION); return; }
    if((video||gif) && (KodaContarAssetsTipo(L"video")+KodaContarAssetsTipo(L"gif"))>=5) { MessageBoxW(owner,L"Este projeto já tem 5 clipes de vídeo/GIF. Exclua um antes de importar outro.",L"Limite de vídeo",MB_OK|MB_ICONINFORMATION); return; }
    std::wstring assetAbs, assetRel;
    if(!KodaCriarArquivoEmAssets(path,assetAbs,assetRel)) {
        MessageBoxW(owner,L"Não foi possível copiar o arquivo para a pasta assets do projeto. Verifique se o projeto pode ser salvo.",L"Importar",MB_OK|MB_ICONERROR);
        return;
    }
    if(audio) {
        KodaAudioFechar();
        g_audioPath=assetAbs; g_audioRelativePath=assetRel;
        g_audioStartSeconds=std::max(0.0,timelinePlayheadSeconds); g_audioDurationSeconds=0.0;
        KodaRegistrarAsset(assetRel,L"audio",g_audioStartSeconds,0.0,frameAtual,selectedTimelineLayer,0,0);
        KodaAudioAbrir(owner,false); // obtém duração quando houver decodificador instalado
        if(g_audioDurationSeconds<=0.0) g_audioDurationSeconds=KodaDetectarDuracaoMCI(assetAbs);
        for(size_t ai=0;ai<g_projectMediaAssets.size();++ai) if(g_projectMediaAssets[ai].relativePath==assetRel) { g_projectMediaAssets[ai].durationSeconds=g_audioDurationSeconds; g_selectedMediaAssetIndex=(int)ai; }
        if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
        SalvarProjetoCompleto();
        return;
    }
    if(image && !gif) {
        HBITMAP imported=KodaCarregarImagemColorida(assetAbs.c_str());
        if(!imported) {
            wchar_t msg[512]; wsprintfW(msg,L"O arquivo foi copiado para assets, mas o decodificador de imagem do Windows não conseguiu abri-lo. WebP depende de um codec instalado.\n\n%s",KodaNomeBase(path).c_str());
            MessageBoxW(owner,msg,L"Importar imagem",MB_OK|MB_ICONWARNING); return;
        }
        KodaAtivarColagemImagem(owner,imported,assetRel,true);
        return;
    }
    // Vídeos e GIFs são registrados como clipes amarelos independentes na timeline.
    // A importação/cópia e a persistência estão implementadas; a decodificação de seus
    // quadros para playback/exportação ainda precisa do renderer de vídeo.
    double dur=KodaDetectarDuracaoMCI(assetAbs);
    if(dur<=0.0) dur=3.0;
    KodaRegistrarAsset(assetRel,gif?L"gif":L"video",timelinePlayheadSeconds,dur,frameAtual,selectedTimelineLayer,0,0);
    g_selectedMediaAssetIndex=(int)g_projectMediaAssets.size()-1;
    if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
    SalvarProjetoCompleto();
}
static void KodaMostrarDialogoImportar(HWND owner) {
    static const wchar_t filters[]=L"Mídias suportadas (*.bmp;*.png;*.jpg;*.jpeg;*.webp;*.tif;*.tiff;*.gif;*.mp3;*.wav;*.opus;*.m4a;*.aac;*.flac;*.ogg;*.wma;*.mp4;*.mov;*.avi;*.wmv;*.mkv;*.webm;*.mpeg;*.mpg;*.m4v;*.3gp)\0*.bmp;*.png;*.jpg;*.jpeg;*.webp;*.tif;*.tiff;*.gif;*.mp3;*.wav;*.opus;*.m4a;*.aac;*.flac;*.ogg;*.wma;*.mp4;*.mov;*.avi;*.wmv;*.mkv;*.webm;*.mpeg;*.mpg;*.m4v;*.3gp\0Imagens (*.bmp;*.png;*.jpg;*.jpeg;*.webp;*.tif;*.tiff;*.gif)\0*.bmp;*.png;*.jpg;*.jpeg;*.webp;*.tif;*.tiff;*.gif\0Áudio (*.mp3;*.wav;*.opus;*.m4a;*.aac;*.flac;*.ogg;*.wma)\0*.mp3;*.wav;*.opus;*.m4a;*.aac;*.flac;*.ogg;*.wma\0Vídeo (*.mp4;*.mov;*.avi;*.wmv;*.mkv;*.webm;*.mpeg;*.mpg;*.m4v;*.3gp)\0*.mp4;*.mov;*.avi;*.wmv;*.mkv;*.webm;*.mpeg;*.mpg;*.m4v;*.3gp\0Todos os arquivos (*.*)\0*.*\0\0";
    wchar_t path[32768]={0};
    OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=owner;
    ofn.lpstrFile=path; ofn.nMaxFile=(DWORD)(sizeof(path)/sizeof(path[0])); ofn.lpstrFilter=filters;
    ofn.lpstrTitle=L"Importar mídia para o projeto Kodanima";
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_ENABLESIZING;
    if(GetOpenFileNameW(&ofn)) KodaImportarArquivo(owner,path);
}
static void KodaColarImagemClipboard(HWND owner) {
    HBITMAP b=KodaObterImagemClipboard();
    if(!b) { MessageBoxW(owner,L"A área de transferência não contém uma imagem compatível. Copie a imagem no navegador e tente Ctrl+Shift+V novamente.",L"Colar imagem",MB_OK|MB_ICONINFORMATION); return; }
    std::wstring abs,rel;
    if(!KodaObterPastaProjeto(abs)) { DeleteObject(b); MessageBoxW(owner,L"Salve/crie o projeto antes de colar uma imagem.",L"Colar imagem",MB_OK|MB_ICONWARNING); return; }
    std::wstring assets=abs+L"\\assets";
    if(GetFileAttributesW(assets.c_str())==INVALID_FILE_ATTRIBUTES) CreateDirectoryW(assets.c_str(),NULL);
    // O conteúdo copiado do navegador é persistido como BMP no projeto para não depender do clipboard após fechar.
    bool saved=false;
    for(unsigned i=1;i<100000;++i) {
        wchar_t leaf[64]; swprintf(leaf,64,L"media_%04u.bmp",i);
        std::wstring dst=assets+L"\\"+leaf;
        if(GetFileAttributesW(dst.c_str())!=INVALID_FILE_ATTRIBUTES) continue;
        if(SalvarHBITMAPComoBMP(b,dst.c_str())) {
            rel=L"assets/"; rel+=leaf; saved=true; break;
        }
    }
    if(!saved) { DeleteObject(b); MessageBoxW(owner,L"Não foi possível salvar a imagem colada na pasta assets.",L"Colar imagem",MB_OK|MB_ICONERROR); return; }
    KodaAtivarColagemImagem(owner,b,rel,true);
}
static void KodaProcessarDrop(HWND hwndDestino,HDROP drop) {
    if(!drop) return;
    UINT count=DragQueryFileW(drop,0xFFFFFFFF,NULL,0);
    HWND owner=(hwndCanvas&&IsWindow(hwndCanvas))?hwndCanvas:((hwndTimeline&&IsWindow(hwndTimeline))?hwndTimeline:hwndDestino);
    // Ao soltar sobre a timeline, use o ponto do drop como novo playhead e selecione
    // a camada de desenho sob o ponteiro antes de importar a mídia.
    if(hwndTimeline && IsWindow(hwndTimeline) && hwndDestino && (hwndDestino==hwndTimeline || IsChild(hwndTimeline,hwndDestino))) {
        POINT dropPt{};
        if(DragQueryPoint(drop,&dropPt)) {
            ClientToScreen(hwndDestino,&dropPt); ScreenToClient(hwndTimeline,&dropPt);
            if(dropPt.x>=TIMELINE_SPLIT_X && dropPt.y>=TIMELINE_RULER_TOP+alturaReguaTimeline) {
                timelinePlayheadSeconds=std::max(0.0,timelineScrollSeconds+(dropPt.x-TIMELINE_SPLIT_X)/46.0);
                double acc=0.0;
                for(int fi=0;fi<(int)listaFrames.size();++fi) {
                    if(!QuadroExisteNaCamada(listaFrames[fi],selectedTimelineLayer)) continue;
                    double d=DuracaoCamadaQuadro(listaFrames[fi],selectedTimelineLayer);
                    if(timelinePlayheadSeconds<acc+d) { IrParaQuadro(fi); break; }
                    acc+=d;
                }
                int row=(dropPt.y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/std::max(1,alturaLinhaCamada)+timelineLayerScroll;
                if(row>=0 && row<(int)timelineLayers.size()) { selectedTimelineLayer=activeTimelineLayer=row; SincronizarAliasCamadaAtiva(); }
                if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
                if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
            }
        }
    }
    // Lotes aceitam áudio/vídeo; somente uma imagem fica em modo de prévia por vez.
    for(UINT i=0;i<count;++i) {
        UINT chars=DragQueryFileW(drop,i,NULL,0);
        if(!chars || chars>32760) continue;
        std::vector<wchar_t> path(chars+1,0);
        if(DragQueryFileW(drop,i,path.data(),chars+1)) KodaImportarArquivo(owner,path.data());
    }
    DragFinish(drop);
}
static LRESULT CALLBACK KodaDropSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam,UINT_PTR id,DWORD_PTR data) {
    (void)id; (void)data;
    if(msg==WM_DROPFILES) { KodaProcessarDrop(hwnd,(HDROP)wParam); return 0; }
    return DefSubclassProc(hwnd,msg,wParam,lParam);
}
static BOOL CALLBACK KodaAtivarDropNosFilhos(HWND child,LPARAM) {
    DragAcceptFiles(child,TRUE);
    SetWindowSubclass(child,KodaDropSubclassProc,0x4B44,0);
    return TRUE;
}

void AlternarPreview() {
    if (listaFrames.size()<1 || !hwndCanvas) return;
    previewAtivo=!previewAtivo;
    if (previewAtivo) {
        const double total=std::max(0.001,TimelineSecondsTotalExact());
        if (timelineLoopEnabled) {
            timelineLoopStartSeconds=std::max(0.0,std::min(total,timelineLoopStartSeconds));
            timelineLoopEndSeconds=std::max(timelineLoopStartSeconds+0.001,std::min(total,timelineLoopEndSeconds));
            if(timelinePlayheadSeconds<timelineLoopStartSeconds || timelinePlayheadSeconds>=timelineLoopEndSeconds)
                timelinePlayheadSeconds=timelineLoopStartSeconds;
        } else if(timelinePlayheadSeconds>=total) timelinePlayheadSeconds=0.0;
        previewTimeSeconds=timelinePlayheadSeconds;
        if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
        if(hwndTimeline) SetWindowTextW(GetDlgItem(hwndTimeline,ID_BTN_PLAY_PREVIEW),L"■ Parar");
        int fpsPreview=std::max(1,configAtual.fps); int ms=std::max(1,1000/fpsPreview);
        SetTimer(hwndCanvas,ID_TIMER_PREVIEW,ms,NULL);
        KodaAudioTocarDaTimeline(previewTimeSeconds);
    } else {
        KillTimer(hwndCanvas,ID_TIMER_PREVIEW);
        KodaAudioParar();
        if(hwndTimeline) SetWindowTextW(GetDlgItem(hwndTimeline,ID_BTN_PLAY_PREVIEW),L"▶ Play");
        timelinePlayheadSeconds=previewTimeSeconds;
        if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
    }
}

void InserirQuadroNoMeio() {
    if(timelineLayers.empty()) return;
    int pos=CriarQuadroNaCamada(selectedTimelineLayer,frameAtual);
    if(pos>=0) IrParaQuadro(pos);
}

void AlternarCanvasMaximizado() {
    // Tab alterna apenas a visibilidade dos dois painéis flutuantes.
    // Não maximiza nem restaura o canvas.
    bool visiveis = (hwndTools && IsWindow(hwndTools) && IsWindowVisible(hwndTools)) ||
                    (hwndTimeline && IsWindow(hwndTimeline) && IsWindowVisible(hwndTimeline));
    int mostrar = visiveis ? SW_HIDE : SW_SHOW;
    if (hwndTools && IsWindow(hwndTools)) {
        ShowWindow(hwndTools, mostrar);
        if (!visiveis) SetWindowPos(hwndTools, HWND_TOPMOST, 0, 0, 0, 0,
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    if (hwndTimeline && IsWindow(hwndTimeline)) {
        ShowWindow(hwndTimeline, mostrar);
        if (!visiveis) SetWindowPos(hwndTimeline, HWND_TOPMOST, 0, 0, 0, 0,
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    if (hwndCanvas && IsWindow(hwndCanvas)) SetFocus(hwndCanvas);
}

void SalvarConfiguracoesUsuario();

// Preferências do usuário ficam em Documentos\Kodanima\config.ini.
static bool GarantirDiretorioKodanima(const wchar_t* path);
void CaminhoConfiguracao(wchar_t* out) {
    wchar_t docs[MAX_PATH] = L"";
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, docs);
    if(!docs[0] || FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, docs))) {
        docs[0]=0;
        SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, docs);
    }
    if(!docs[0]) { lstrcpynW(out,L"config.ini",MAX_PATH); return; }
    wchar_t dir[MAX_PATH]={0};
    wsprintfW(dir,L"%s\\Kodanima",docs);
    if(!GarantirDiretorioKodanima(dir)) {
        wchar_t local[MAX_PATH]={0};
        if(SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,NULL,SHGFP_TYPE_CURRENT,local)))
            wsprintfW(dir,L"%s\\Kodanima",local);
    }
    lstrcpynW(out,dir,MAX_PATH);
    if(!GarantirDiretorioKodanima(out)) lstrcpynW(out,docs,MAX_PATH);
    lstrcatW(out,L"\\config.ini");
}

static void PersistirDiagnosticoCPUIni() {
    wchar_t ini[MAX_PATH], num[32]; CaminhoConfiguracao(ini);
    WritePrivateProfileStringW(L"DesempenhoCPU",L"Fornecedor",g_kodaCPU.vendor,ini);
    WritePrivateProfileStringW(L"DesempenhoCPU",L"Modelo",g_kodaCPU.brand,ini);
    wsprintfW(num,L"%d",g_kodaCPU.detectedCount); WritePrivateProfileStringW(L"DesempenhoCPU",L"RecursosDetectados",num,ini);
    const wchar_t* kernel=g_kodaCPU.pixelKernel==2?L"AVX2":g_kodaCPU.pixelKernel==1?L"SSE2":L"Escalar";
    WritePrivateProfileStringW(L"DesempenhoCPU",L"KernelPixelsAtivo",kernel,ini);
    const struct { const wchar_t* name; bool value; } flags[]={
        {L"MMX",g_kodaCPU.mmx},{L"SSE",g_kodaCPU.sse},{L"SSE2",g_kodaCPU.sse2},
        {L"SSE3",g_kodaCPU.sse3},{L"SSSE3",g_kodaCPU.ssse3},{L"SSE41",g_kodaCPU.sse41},
        {L"SSE42",g_kodaCPU.sse42},{L"AVX",g_kodaCPU.avx},{L"AVX2",g_kodaCPU.avx2},
        {L"AVXUsavel",g_kodaCPU.avxUsable},{L"AVX2Usavel",g_kodaCPU.avx2Usable},
        {L"AES",g_kodaCPU.aes},{L"IntelVTX",g_kodaCPU.vmx},{L"AMDV",g_kodaCPU.svm}
    };
    for(size_t i=0;i<sizeof(flags)/sizeof(flags[0]);++i)
        WritePrivateProfileStringW(L"DesempenhoCPU",flags[i].name,flags[i].value?L"1":L"0",ini);
    WritePrivateProfileStringW(NULL,NULL,NULL,ini);
}

static std::wstring MinusculaTablet(std::wstring v) {
    std::transform(v.begin(),v.end(),v.begin(),[](wchar_t c){ return (wchar_t)towlower(c); });
    return v;
}
static int InferirNiveisPerfilTablet(const std::wstring& identity, std::wstring& source) {
    const std::wstring t=MinusculaTablet(identity);
    if(t.find(L"16384")!=std::wstring::npos || t.find(L"16k")!=std::wstring::npos) { source=L"Perfil estimado pelo nome/modelo (16K)"; return 16384; }
    if(t.find(L"8192")!=std::wstring::npos || t.find(L"8k")!=std::wstring::npos) { source=L"Perfil estimado pelo nome/modelo (8K)"; return 8192; }
    if(t.find(L"4096")!=std::wstring::npos || t.find(L"4k")!=std::wstring::npos) { source=L"Perfil estimado pelo nome/modelo (4K)"; return 4096; }
    if(t.find(L"2048")!=std::wstring::npos || t.find(L"2k")!=std::wstring::npos) { source=L"Perfil estimado pelo nome/modelo (2K)"; return 2048; }
    if(t.find(L"1024")!=std::wstring::npos || t.find(L"1k")!=std::wstring::npos) { source=L"Perfil estimado pelo nome/modelo (1K)"; return 1024; }
    // Biblioteca inicial de fabricante. É apenas fallback: modelos do mesmo fabricante
    // podem ter resoluções distintas; o eixo DVC_NPRESSURE sempre tem precedência.
    if(t.find(L"wacom intuos pro")!=std::wstring::npos || t.find(L"cintiq pro")!=std::wstring::npos) { source=L"Perfil estimado Wacom Pro (confira o modelo)"; return 8192; }
    if(t.find(L"wacom")!=std::wstring::npos) { source=L"Perfil estimado Wacom (confira o modelo)"; return 2048; }
    if(t.find(L"xp-pen")!=std::wstring::npos || t.find(L"xppen")!=std::wstring::npos ||
       t.find(L"huion")!=std::wstring::npos || t.find(L"gaomon")!=std::wstring::npos ||
       t.find(L"veikk")!=std::wstring::npos || t.find(L"ugee")!=std::wstring::npos ||
       t.find(L"artisul")!=std::wstring::npos || t.find(L"parblo")!=std::wstring::npos) {
        source=L"Perfil estimado pelo fabricante (confira o modelo)"; return 8192;
    }
    if(t.find(L"tablet")!=std::wstring::npos || t.find(L"digitizer")!=std::wstring::npos ||
       t.find(L"digitalizador")!=std::wstring::npos || t.find(L"pen")!=std::wstring::npos) {
        source=L"Perfil genérico estimado; Windows Pointer normaliza até 1024"; return 1024;
    }
    source=L"Níveis físicos não informados pelo driver"; return 0;
}
static std::wstring TabletPropString(HMODULE setup, HDEVINFO set, PSP_DEVINFO_DATA dev, DWORD property) {
    typedef BOOL (WINAPI *FnGetPropW)(HDEVINFO,PSP_DEVINFO_DATA,DWORD,PDWORD,PBYTE,DWORD,PDWORD);
    static FnGetPropW getProp=NULL; static HMODULE cachedSetup=NULL;
    if(cachedSetup!=setup) { cachedSetup=setup; getProp=(FnGetPropW)GetProcAddress(setup,"SetupDiGetDeviceRegistryPropertyW"); }
    if(!getProp || !set || !dev) return std::wstring();
    BYTE data[8192]{}; DWORD type=0,needed=0;
    if(!getProp(set,dev,property,&type,data,sizeof(data),&needed) || (type!=REG_SZ && type!=REG_EXPAND_SZ && type!=REG_MULTI_SZ)) return std::wstring();
    data[sizeof(data)-sizeof(wchar_t)]=0; data[sizeof(data)-1]=0;
    const wchar_t* p=(const wchar_t*)data;
    return p?std::wstring(p):std::wstring(); // first member for MULTI_SZ is enough for the model signature
}
static std::wstring TabletLerValorRegString(HMODULE adv, HKEY key, const wchar_t* valueName) {
    typedef LONG (WINAPI *FnQueryValueW)(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
    static FnQueryValueW query=NULL; static HMODULE cachedAdv=NULL;
    if(cachedAdv!=adv) { cachedAdv=adv; query=(FnQueryValueW)GetProcAddress(adv,"RegQueryValueExW"); }
    if(!query || !key || !valueName) return std::wstring();
    DWORD type=0,bytes=0; if(query(key,valueName,NULL,&type,NULL,&bytes)!=ERROR_SUCCESS || (type!=REG_SZ && type!=REG_EXPAND_SZ) || bytes<sizeof(wchar_t) || bytes>8192) return std::wstring();
    std::vector<wchar_t> buf(bytes/sizeof(wchar_t)+2,0);
    if(query(key,valueName,NULL,&type,(LPBYTE)buf.data(),&bytes)!=ERROR_SUCCESS) return std::wstring();
    buf.back()=0; return std::wstring(buf.data());
}
static void AtualizarPainelTablet() {
    if(g_tabletDeviceValueHwnd && IsWindow(g_tabletDeviceValueHwnd)) {
        std::wstring device=g_tabletManualDeviceName.empty()?g_tabletDetectedName:(g_tabletManualDeviceName+L" (perfil manual; detectado: "+g_tabletDetectedName+L")");
        SetWindowTextW(g_tabletDeviceValueHwnd,device.c_str());
    }
    if(g_tabletDriverValueHwnd && IsWindow(g_tabletDriverValueHwnd)) {
        std::wstring driver=g_tabletDriverName+L" | versão: "+g_tabletDriverVersion;
        if(g_tabletManufacturer.size() && g_tabletManufacturer!=L"Não informado") driver+=L" | fabricante: "+g_tabletManufacturer;
        SetWindowTextW(g_tabletDriverValueHwnd,driver.c_str());
    }
    if(g_tabletLevelsValueHwnd && IsWindow(g_tabletLevelsValueHwnd)) {
        wchar_t n[32];
        int lv=g_tabletManualPressureLevels>=2?g_tabletManualPressureLevels:g_tabletPressureLevelsDetected;
        if(lv>=2) wsprintfW(n,L"%d níveis",lv); else lstrcpyW(n,L"Não informado");
        std::wstring info=n; info+=L" — "; info+=g_tabletManualPressureLevels>=2?L"perfil manual":g_tabletPressureLevelsSource;
        if(pressureApi==1) info+=L"; Windows Pointer fornece pressão normalizada 0–1024, não a contagem física do hardware";
        else if(!g_wintabPressureAxisAvailable) info+=L"; contagem exata requer eixo de pressão Wintab";
        SetWindowTextW(g_tabletLevelsValueHwnd,info.c_str());
    }
    if(hwndSettings && IsWindow(hwndSettings)) {
        HWND editName=GetDlgItem(hwndSettings,ID_SETTINGS_TABLET_NAME_EDIT);
        HWND editLevels=GetDlgItem(hwndSettings,ID_SETTINGS_TABLET_LEVELS_EDIT);
        if(editName && GetFocus()!=editName) SetWindowTextW(editName,g_tabletManualDeviceName.c_str());
        if(editLevels && GetFocus()!=editLevels) { wchar_t v[32]; wsprintfW(v,L"%d",g_tabletManualPressureLevels); SetWindowTextW(editLevels,v); }
    }
}
static void DetectarMesaDigitalizadoraUmaVez() {
    if(g_tabletSetupApiScanDone) return;
    g_tabletSetupApiScanDone=true;

    // Primeiro tenta descobrir a faixa física Wintab mesmo antes da janela do canvas
    // existir. WTInfo não exige abrir um contexto; isso permite exibir os níveis reais
    // logo que o usuário abrir Preferências > Tablet.
    if(!g_wintabModule) g_wintabModule=LoadLibraryW(L"Wintab32.dll");
    if(g_wintabModule) {
        if(!g_wtInfoA) g_wtInfoA=(KOD_WTInfoA)GetProcAddress(g_wintabModule,"WTInfoA");
        UINT device=0xFFFFFFFFu; KOD_LOGCONTEXTA defContext{};
        if(g_wtInfoA && g_wtInfoA(KOD_WTI_DEFSYSCTX,0,&defContext) && defContext.lcDevice!=0xFFFFFFFFu) device=defContext.lcDevice;
        KOD_WT_AXIS axis{}; bool found=false;
        if(device!=0xFFFFFFFFu && WintabLerEixoPressao(device,&axis)) { found=true; g_tabletWintabDeviceIndex=(int)device; }
        if(!found && g_wtInfoA) {
            for(UINT dev=0;dev<16;++dev) if(WintabLerEixoPressao(dev,&axis)) { found=true; g_tabletWintabDeviceIndex=(int)dev; break; }
        }
        if(found) {
            g_wintabPressureMin=axis.axMin; g_wintabPressureMax=axis.axMax; g_wintabPressureAxisAvailable=true;
            long long levels=(long long)axis.axMax-(long long)axis.axMin+1LL;
            if(levels>=2 && levels<=65536) {
                g_tabletPressureLevelsDetected=(int)levels;
                g_tabletPressureLevelsSource=L"Eixo físico consultado por WTInfoA / Wintab32.dll";
                g_tabletLevelsAreEstimated=false;
            }
        }
    }

    HMODULE setup=LoadLibraryW(L"setupapi.dll");
    if(!setup) { g_tabletPressureLevelsSource=L"SetupAPI indisponível; use perfil manual"; AtualizarPainelTablet(); return; }
    typedef HDEVINFO (WINAPI *FnGetClassDevsW)(const GUID*,LPCWSTR,HWND,DWORD);
    typedef BOOL (WINAPI *FnEnumDeviceInfo)(HDEVINFO,DWORD,PSP_DEVINFO_DATA);
    typedef BOOL (WINAPI *FnDestroySet)(HDEVINFO);
    FnGetClassDevsW getSet=(FnGetClassDevsW)GetProcAddress(setup,"SetupDiGetClassDevsW");
    FnEnumDeviceInfo enumDev=(FnEnumDeviceInfo)GetProcAddress(setup,"SetupDiEnumDeviceInfo");
    FnDestroySet destroySet=(FnDestroySet)GetProcAddress(setup,"SetupDiDestroyDeviceInfoList");
    HDEVINFO set=(getSet?getSet(NULL,NULL,NULL,DIGCF_PRESENT|DIGCF_ALLCLASSES):INVALID_HANDLE_VALUE);
    int bestScore=0; std::wstring bestName,bestMfg,bestDriver,bestVersion,bestIdentity;
    HMODULE cfgmgr=LoadLibraryW(L"cfgmgr32.dll");
    typedef ULONG (WINAPI *FnCMGetDevNodeStatus)(PULONG,PULONG,ULONG,ULONG);
    FnCMGetDevNodeStatus cmGetStatus=cfgmgr?(FnCMGetDevNodeStatus)GetProcAddress(cfgmgr,"CM_Get_DevNode_Status"):NULL;
    HMODULE adv=LoadLibraryW(L"advapi32.dll");
    typedef LONG (WINAPI *FnRegOpenKeyExW)(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);
    typedef LONG (WINAPI *FnRegCloseKey)(HKEY);
    FnRegOpenKeyExW regOpen=adv?(FnRegOpenKeyExW)GetProcAddress(adv,"RegOpenKeyExW"):NULL;
    FnRegCloseKey regClose=adv?(FnRegCloseKey)GetProcAddress(adv,"RegCloseKey"):NULL;
    if(set!=INVALID_HANDLE_VALUE && enumDev) {
        for(DWORD index=0;index<2048;++index) {
            SP_DEVINFO_DATA dev{}; dev.cbSize=sizeof(dev);
            if(!enumDev(set,index,&dev)) break;
            DWORD configFlags=0,required=0,type=0;
            typedef BOOL (WINAPI *FnGetPropW)(HDEVINFO,PSP_DEVINFO_DATA,DWORD,PDWORD,PBYTE,DWORD,PDWORD);
            FnGetPropW getProp=(FnGetPropW)GetProcAddress(setup,"SetupDiGetDeviceRegistryPropertyW");
            if(getProp && getProp(set,&dev,SPDRP_CONFIGFLAGS,&type,(PBYTE)&configFlags,sizeof(configFlags),&required) && type==REG_DWORD && (configFlags&0x00000001u)) continue;
            std::wstring desc=TabletPropString(setup,set,&dev,SPDRP_DEVICEDESC);
            std::wstring mfg=TabletPropString(setup,set,&dev,SPDRP_MFG);
            std::wstring cls=TabletPropString(setup,set,&dev,SPDRP_CLASS);
            std::wstring hwid=TabletPropString(setup,set,&dev,SPDRP_HARDWAREID);
            std::wstring driverKey=TabletPropString(setup,set,&dev,SPDRP_DRIVER);
            std::wstring identity=desc+L" "+mfg+L" "+cls+L" "+hwid;
            const std::wstring low=MinusculaTablet(identity);
            int score=0;
            const wchar_t* brands[]={L"wacom",L"huion",L"xp-pen",L"xppen",L"gaomon",L"veikk",L"ugee",L"artisul",L"parblo",L"bosto",L"xencelabs"};
            for(size_t bi=0;bi<sizeof(brands)/sizeof(brands[0]);++bi) if(low.find(brands[bi])!=std::wstring::npos) { score+=40; break; }
            if(low.find(L"tablet")!=std::wstring::npos || low.find(L"digitizer")!=std::wstring::npos || low.find(L"digitalizador")!=std::wstring::npos) score+=25;
            if(low.find(L"pen")!=std::wstring::npos || low.find(L"stylus")!=std::wstring::npos) score+=12;
            if(low.find(L"touchpad")!=std::wstring::npos || low.find(L"keyboard")!=std::wstring::npos) score-=50;
            if(low.find(L"mouse")!=std::wstring::npos && low.find(L"pen")==std::wstring::npos) score-=15;
            if(score<=bestScore) continue;
            // Se o Configuration Manager conseguir consultar o devnode, só considera
            // ativos os dispositivos com DN_STARTED. Se a chamada não estiver disponível,
            // mantém o filtro de dispositivos presentes e não desabilitados do SetupAPI.
            if(cmGetStatus) {
                ULONG devStatus=0,problemCode=0;
                const ULONG cr=cmGetStatus(&devStatus,&problemCode,(ULONG)dev.DevInst,0);
                if(cr==0 && (devStatus & 0x00000008u)==0) continue; // DN_STARTED
            }
            std::wstring driverName,driverVersion;
            if(!driverKey.empty() && regOpen && adv) {
                std::wstring keyPath=L"SYSTEM\\CurrentControlSet\\Control\\Class\\"+driverKey;
                HKEY key=NULL;
                if(regOpen(HKEY_LOCAL_MACHINE,keyPath.c_str(),0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS && key) {
                    driverName=TabletLerValorRegString(adv,key,L"DriverDesc");
                    driverVersion=TabletLerValorRegString(adv,key,L"DriverVersion");
                    std::wstring provider=TabletLerValorRegString(adv,key,L"ProviderName");
                    if(!provider.empty()) mfg=provider;
                    if(regClose) regClose(key);
                }
            }
            bestScore=score; bestName=desc.empty()?mfg:desc; bestMfg=mfg; bestDriver=driverName.empty()?desc:driverName; bestVersion=driverVersion.empty()?L"Não informada no registro do driver":driverVersion; bestIdentity=identity+L" "+driverName+L" "+driverVersion;
        }
    }
    if(set!=INVALID_HANDLE_VALUE && destroySet) destroySet(set);
    if(adv) FreeLibrary(adv);
    if(cfgmgr) FreeLibrary(cfgmgr);
    FreeLibrary(setup);
    if(bestScore>0) {
        g_tabletDetectedName=bestName; g_tabletManufacturer=bestMfg.empty()?L"Não informado":bestMfg; g_tabletDriverName=bestDriver.empty()?L"Não identificado":bestDriver; g_tabletDriverVersion=bestVersion.empty()?L"Não informada":bestVersion;
        std::wstring source; int levels=InferirNiveisPerfilTablet(bestIdentity,source);
        if(!g_wintabPressureAxisAvailable) { g_tabletPressureLevelsDetected=levels; g_tabletPressureLevelsSource=source; g_tabletLevelsAreEstimated=(levels>0); }
    } else {
        g_tabletDetectedName=L"Nenhuma mesa identificada pelo SetupAPI";
        g_tabletManufacturer=L"Não informado"; g_tabletDriverName=L"Não identificado"; g_tabletDriverVersion=L"Não informada";
        if(!g_tabletPressureLevelsDetected) g_tabletPressureLevelsSource=L"Sem detecção; use nome e níveis manuais";
    }
    // O nome manual também pode ser uma chave da biblioteca de perfis embutida.
    if(!g_wintabPressureAxisAvailable && g_tabletManualPressureLevels==0 && !g_tabletManualDeviceName.empty()) {
        std::wstring manualSource; int manualGuess=InferirNiveisPerfilTablet(g_tabletManualDeviceName,manualSource);
        if(manualGuess>=2) { g_tabletPressureLevelsDetected=manualGuess; g_tabletPressureLevelsSource=manualSource; g_tabletLevelsAreEstimated=true; }
    }
    AtualizarPainelTablet();
}
void CarregarConfiguracoesUsuario() {
    wchar_t ini[MAX_PATH], imagens[MAX_PATH] = L"";
    CaminhoConfiguracao(ini);
    { wchar_t v[MAX_PATH]=L""; GetPrivateProfileStringW(L"Tablet",L"NomeManual",L"",v,MAX_PATH,ini); g_tabletManualDeviceName=v; }
    g_tabletManualPressureLevels=GetPrivateProfileIntW(L"Tablet",L"NiveisManual",0,ini);
    if(g_tabletManualPressureLevels<2 || g_tabletManualPressureLevels>65536) g_tabletManualPressureLevels=0;
    for(int i=0;i<g_kodaShortcutCount;++i){ wchar_t key[32]; wsprintfW(key,L"Atalho%d",i); UINT v=(UINT)GetPrivateProfileIntW(L"Atalhos",key,(int)g_kodaShortcuts[i].defaultVk,ini); if(!g_kodaShortcuts[i].fixed && v>0 && v<256) g_kodaShortcuts[i].vk=v; }
    SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, imagens);
    if (!imagens[0]) { SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, imagens); }
    GetPrivateProfileStringW(L"Usuario", L"PastaProjetos", imagens, configAtual.diretorioPasta, MAX_PATH, ini);
    { wchar_t lang[32]=L"pt-BR"; GetPrivateProfileStringW(L"Usuario",L"Idioma",L"pt-BR",lang,32,ini); g_languageCode=lang; }
    // O arquivo de áudio e sua posição são dados do projeto; nunca os carregamos do INI global.
    g_audioScrubEnabled=GetPrivateProfileIntW(L"Audio",L"ScrubAtivo",1,ini)!=0;
    temaAtual = GetPrivateProfileIntW(L"Usuario", L"Tema", 0, ini);
    wchar_t motorSalvo[16]={0};
    GetPrivateProfileStringW(L"Usuario",L"MotorGrafico",L"",motorSalvo,16,ini);
    rendererPreferido = motorSalvo[0] ? _wtoi(motorSalvo) : DetectarMelhorMotorGrafico();
    escalaInterface = GetPrivateProfileIntW(L"Usuario", L"EscalaInterface", 100, ini);
    tamanhoTexto = GetPrivateProfileIntW(L"Usuario", L"TamanhoTexto", 100, ini);
    tamanhoBotoesTimeline = GetPrivateProfileIntW(L"Usuario", L"TamanhoBotoesTimeline", 80, ini);
    if (tamanhoBotoesTimeline != 70 && tamanhoBotoesTimeline != 80 && tamanhoBotoesTimeline != 90 && tamanhoBotoesTimeline != 100) tamanhoBotoesTimeline = 80;
    alturaLinhaCamada=GetPrivateProfileIntW(L"Usuario",L"AlturaLinhaCamada",27,ini); if(alturaLinhaCamada!=22 && alturaLinhaCamada!=27 && alturaLinhaCamada!=32 && alturaLinhaCamada!=38) alturaLinhaCamada=27;
    alturaReguaTimeline=GetPrivateProfileIntW(L"Usuario",L"AlturaReguaTimeline",24,ini); if(alturaReguaTimeline!=18 && alturaReguaTimeline!=24 && alturaReguaTimeline!=30 && alturaReguaTimeline!=36) alturaReguaTimeline=24;
    nucleosDesejados = GetPrivateProfileIntW(L"Usuario", L"Nucleos", 0, ini);
    limiteRAMMB = GetPrivateProfileIntW(L"Usuario", L"LimiteRAMMB", 0, ini);
    if (rendererPreferido < 0 || rendererPreferido > 3) rendererPreferido = 0;
    if (escalaInterface != 80 && escalaInterface != 100 && escalaInterface != 125 && escalaInterface != 150) escalaInterface = 100;
    if (tamanhoTexto != 80 && tamanhoTexto != 100 && tamanhoTexto != 125 && tamanhoTexto != 150) tamanhoTexto = 100;
    if (nucleosDesejados < 0 || nucleosDesejados > 256) nucleosDesejados = 0;
    if (limiteRAMMB != 0 && limiteRAMMB != 512 && limiteRAMMB != 1024 && limiteRAMMB != 2048 && limiteRAMMB != 4096 && limiteRAMMB != 8192) limiteRAMMB = 0;
    historyActionLimit=GetPrivateProfileIntW(L"Usuario",L"HistoricoAcoes",20,ini);
    if(historyActionLimit!=5 && historyActionLimit!=10 && historyActionLimit!=20 && historyActionLimit!=30 && historyActionLimit!=50) historyActionLimit=20;
    historyCacheLimitMB=GetPrivateProfileIntW(L"Usuario",L"HistoricoCacheMB",200,ini);
    if(historyCacheLimitMB!=16 && historyCacheLimitMB!=32 && historyCacheLimitMB!=64 && historyCacheLimitMB!=128 && historyCacheLimitMB!=200) historyCacheLimitMB=200;
    { wchar_t f[64]=L"2"; GetPrivateProfileStringW(L"Usuario",L"TamanhoLapis",L"2",f,64,ini); tamanhoLapis=(float)wcstod(f,NULL); }
    nivelGapBalde = GetPrivateProfileIntW(L"Usuario", L"GapBalde", 8, ini);
    toleranciaBalde = GetPrivateProfileIntW(L"Usuario", L"ToleranciaBalde", 3, ini);
    pressaoCanetaAtiva = GetPrivateProfileIntW(L"Usuario", L"PressaoCaneta", 0, ini) != 0;
    pressureApi = GetPrivateProfileIntW(L"Usuario", L"APIPressaoCaneta", 0, ini);
    { wchar_t f[64]=L"0.01"; GetPrivateProfileStringW(L"Usuario",L"PressaoMinima",L"0.01",f,64,ini); tamanhoMinimoPressao=(float)wcstod(f,NULL); }
    { wchar_t f[64]=L"12"; GetPrivateProfileStringW(L"Usuario",L"PressaoMaxima",L"12",f,64,ini); tamanhoMaximoPressao=(float)wcstod(f,NULL); }
    lapisAntiAlias=GetPrivateProfileIntW(L"Usuario",L"LapisAntiAlias",1,ini)!=0;
    estabilizadorTracoAtivo=GetPrivateProfileIntW(L"Usuario",L"EstabilizadorTracoAtivo",1,ini)!=0;
    nivelEstabilizadorTraco=std::max(0,std::min(100,(int)GetPrivateProfileIntW(L"Usuario",L"NivelEstabilizadorTraco",35,ini)));
    durezaPincel=std::max(0,std::min(3,(int)GetPrivateProfileIntW(L"Usuario",L"DurezaPincel",3,ini)));
    if (pressureApi < 0 || pressureApi > 1) pressureApi = 0;
    GetPrivateProfileStringW(L"Usuario", L"Cursor", L"", caminhoCursor, MAX_PATH, ini);
    if (temaAtual < 0 || temaAtual > 3) temaAtual = 0;
    tamanhoLapis = std::max(0.01f, std::min(20000.0f, tamanhoLapis));
    tamanhoMinimoPressao=std::max(0.01f,std::min(5000.0f,tamanhoMinimoPressao));
    tamanhoMaximoPressao=std::max(tamanhoMinimoPressao,std::min(5000.0f,tamanhoMaximoPressao));
    tamanhoBorracha = GetPrivateProfileIntW(L"Usuario",L"TamanhoBorracha",12,ini);
    pressaoBorrachaAtiva = GetPrivateProfileIntW(L"Usuario",L"PressaoBorracha",1,ini)!=0;
    pressaoMinBorracha = GetPrivateProfileIntW(L"Usuario",L"PressaoMinBorracha",2,ini);
    pressaoMaxBorracha = GetPrivateProfileIntW(L"Usuario",L"PressaoMaxBorracha",36,ini);
    modoLaco = GetPrivateProfileIntW(L"Usuario",L"ModoLaco",1,ini)==1?1:0;
    corSelecao = (COLORREF)GetPrivateProfileIntW(L"Usuario",L"CorSelecao",(int)RGB(40,150,255),ini);
    tamanhoBorracha=std::max(1,std::min(100,tamanhoBorracha));
    pressaoMinBorracha=std::max(1,std::min(100,pressaoMinBorracha));
    pressaoMaxBorracha=std::max(pressaoMinBorracha,std::min(100,pressaoMaxBorracha));
    if (nivelGapBalde < 0) nivelGapBalde = 0;
    if (toleranciaBalde < 0) toleranciaBalde = 0;
    if (toleranciaBalde > 20) toleranciaBalde = 20;
    if (caminhoCursor[0]) {
        HCURSOR c = LoadCursorFromFileW(caminhoCursor);
        if (c) cursorPersonalizado = c;
    }
    // Garante um INI inicial com os valores padrão quando o programa é aberto pela primeira vez.
    DWORD atributosIni = GetFileAttributesW(ini);
    if (atributosIni == INVALID_FILE_ATTRIBUTES) SalvarConfiguracoesUsuario();
    dpiProjeto = GetPrivateProfileIntW(L"Projeto", L"DPI", 96, ini);
    if(dpiProjeto<36 || dpiProjeto>1200) dpiProjeto=96;
    ultimoProjetoW=GetPrivateProfileIntW(L"Projeto",L"Largura",800,ini);
    ultimoProjetoH=GetPrivateProfileIntW(L"Projeto",L"Altura",600,ini);
    ultimoProjetoFPS=GetPrivateProfileIntW(L"Projeto",L"FPS",24,ini);
    timelineSegueTema=GetPrivateProfileIntW(L"Timeline",L"SegueTema",1,ini)!=0;
    corTimelinePersonalizada=(COLORREF)GetPrivateProfileIntW(L"Timeline",L"CorPersonalizada",(int)RGB(42,46,54),ini);
}
void SalvarConfiguracoesUsuario() {
    wchar_t ini[MAX_PATH], num[32]; CaminhoConfiguracao(ini);
    for(int i=0;i<g_kodaShortcutCount;++i){ wchar_t key[32]; wsprintfW(key,L"Atalho%d",i); wsprintfW(num,L"%u",(unsigned)(g_kodaShortcuts[i].fixed?g_kodaShortcuts[i].defaultVk:g_kodaShortcuts[i].vk)); WritePrivateProfileStringW(L"Atalhos",key,num,ini); }
    WritePrivateProfileStringW(L"Usuario", L"PastaProjetos", configAtual.diretorioPasta, ini);
    wsprintfW(num, L"%d", temaAtual); WritePrivateProfileStringW(L"Usuario", L"Tema", num, ini);
    wsprintfW(num, L"%d", rendererPreferido); WritePrivateProfileStringW(L"Usuario", L"MotorGrafico", num, ini);
    wsprintfW(num, L"%d", escalaInterface); WritePrivateProfileStringW(L"Usuario", L"EscalaInterface", num, ini);
    wsprintfW(num, L"%d", tamanhoTexto); WritePrivateProfileStringW(L"Usuario", L"TamanhoTexto", num, ini);
    wsprintfW(num, L"%d", tamanhoBotoesTimeline); WritePrivateProfileStringW(L"Usuario", L"TamanhoBotoesTimeline", num, ini);
    wsprintfW(num,L"%d",alturaLinhaCamada); WritePrivateProfileStringW(L"Usuario",L"AlturaLinhaCamada",num,ini);
    wsprintfW(num,L"%d",alturaReguaTimeline); WritePrivateProfileStringW(L"Usuario",L"AlturaReguaTimeline",num,ini);
    wsprintfW(num, L"%d", nucleosDesejados); WritePrivateProfileStringW(L"Usuario", L"Nucleos", num, ini);
    wsprintfW(num, L"%d", limiteRAMMB); WritePrivateProfileStringW(L"Usuario", L"LimiteRAMMB", num, ini);
    wsprintfW(num, L"%d", historyActionLimit); WritePrivateProfileStringW(L"Usuario", L"HistoricoAcoes", num, ini);
    wsprintfW(num, L"%d", historyCacheLimitMB); WritePrivateProfileStringW(L"Usuario", L"HistoricoCacheMB", num, ini);
    swprintf(num, 32, L"%.3f", (double)tamanhoLapis); WritePrivateProfileStringW(L"Usuario", L"TamanhoLapis", num, ini);
    wsprintfW(num,L"%d",tamanhoBorracha); WritePrivateProfileStringW(L"Usuario",L"TamanhoBorracha",num,ini);
    wsprintfW(num,L"%d",pressaoBorrachaAtiva?1:0); WritePrivateProfileStringW(L"Usuario",L"PressaoBorracha",num,ini);
    wsprintfW(num,L"%d",pressaoMinBorracha); WritePrivateProfileStringW(L"Usuario",L"PressaoMinBorracha",num,ini);
    wsprintfW(num,L"%d",pressaoMaxBorracha); WritePrivateProfileStringW(L"Usuario",L"PressaoMaxBorracha",num,ini);
    wsprintfW(num,L"%d",modoLaco); WritePrivateProfileStringW(L"Usuario",L"ModoLaco",num,ini);
    wsprintfW(num,L"%u",(unsigned int)corSelecao); WritePrivateProfileStringW(L"Usuario",L"CorSelecao",num,ini);
    wsprintfW(num, L"%d", nivelGapBalde); WritePrivateProfileStringW(L"Usuario", L"GapBalde", num, ini);
    wsprintfW(num, L"%d", toleranciaBalde); WritePrivateProfileStringW(L"Usuario", L"ToleranciaBalde", num, ini);
    wsprintfW(num, L"%d", pressaoCanetaAtiva ? 1 : 0); WritePrivateProfileStringW(L"Usuario", L"PressaoCaneta", num, ini);
    wsprintfW(num, L"%d", pressureApi); WritePrivateProfileStringW(L"Usuario", L"APIPressaoCaneta", num, ini);
    swprintf(num, 32, L"%.3f", (double)tamanhoMinimoPressao); WritePrivateProfileStringW(L"Usuario", L"PressaoMinima", num, ini);
    swprintf(num, 32, L"%.3f", (double)tamanhoMaximoPressao); WritePrivateProfileStringW(L"Usuario", L"PressaoMaxima", num, ini);
    wsprintfW(num,L"%d",lapisAntiAlias?1:0); WritePrivateProfileStringW(L"Usuario",L"LapisAntiAlias",num,ini);
    wsprintfW(num,L"%d",estabilizadorTracoAtivo?1:0); WritePrivateProfileStringW(L"Usuario",L"EstabilizadorTracoAtivo",num,ini);
    wsprintfW(num,L"%d",nivelEstabilizadorTraco); WritePrivateProfileStringW(L"Usuario",L"NivelEstabilizadorTraco",num,ini);
    WritePrivateProfileStringW(L"Usuario",L"Idioma",g_languageCode.c_str(),ini);
    // Somente a preferência global de scrub fica no INI. A faixa de áudio pertence a project.json.
    wsprintfW(num,L"%d",g_audioScrubEnabled?1:0); WritePrivateProfileStringW(L"Audio",L"ScrubAtivo",num,ini);
    wsprintfW(num,L"%d",durezaPincel); WritePrivateProfileStringW(L"Usuario",L"DurezaPincel",num,ini);
    WritePrivateProfileStringW(L"Usuario", L"Cursor", caminhoCursor, ini);
    // Perfil da mesa: nome e níveis manuais (0 = detecção/estimativa automática).
    WritePrivateProfileStringW(L"Tablet",L"NomeManual",g_tabletManualDeviceName.c_str(),ini);
    wsprintfW(num,L"%d",g_tabletManualPressureLevels); WritePrivateProfileStringW(L"Tablet",L"NiveisManual",num,ini);
    // Versão do formato para permitir ampliar as preferências sem perder as antigas.
    WritePrivateProfileStringW(L"Usuario", L"VersaoConfiguracao", L"3", ini);
    // Força a gravação das alterações pendentes no arquivo INI.
    WritePrivateProfileStringW(NULL, NULL, NULL, ini);
    wsprintfW(num,L"%d",dpiProjeto); WritePrivateProfileStringW(L"Projeto",L"DPI",num,ini);
    wsprintfW(num,L"%d",ultimoProjetoW); WritePrivateProfileStringW(L"Projeto",L"Largura",num,ini);
    wsprintfW(num,L"%d",ultimoProjetoH); WritePrivateProfileStringW(L"Projeto",L"Altura",num,ini);
    wsprintfW(num,L"%d",ultimoProjetoFPS); WritePrivateProfileStringW(L"Projeto",L"FPS",num,ini);
    wsprintfW(num,L"%d",timelineSegueTema?1:0); WritePrivateProfileStringW(L"Timeline",L"SegueTema",num,ini);
    wsprintfW(num,L"%u",(unsigned int)corTimelinePersonalizada); WritePrivateProfileStringW(L"Timeline",L"CorPersonalizada",num,ini);
    PersistirDiagnosticoCPUIni();
    if (caminhoProjetoAtivo[0]) {
        wchar_t cp[MAX_PATH]; wsprintfW(cp,L"%s\\config.json",caminhoProjetoAtivo);
        std::wofstream cfg(cp,std::ios::out|std::ios::trunc); cfg.imbue(std::locale::classic());
        if(cfg.is_open()) cfg << L"{\n  \"tool\": \"" << (wchar_t)ferramentaAtual << L"\",\n"
          << L"  \"toolSelectedId\": " << ferramentaSelecionadaID << L",\n"
          << L"  \"pencilSize\": " << tamanhoLapis << L",\n  \"eraserSize\": " << tamanhoBorracha << L",\n"
          << L"  \"pencilPressure\": " << (pressaoCanetaAtiva?L"true":L"false") << L",\n"
          << L"  \"eraserPressure\": " << (pressaoBorrachaAtiva?L"true":L"false") << L",\n"
          << L"  \"pencilMin\": " << tamanhoMinimoPressao << L",\n  \"pencilMax\": " << tamanhoMaximoPressao << L",\n"
          << L"  \"pencilAntiAlias\": " << (lapisAntiAlias?L"true":L"false") << L",\n  \"pencilHardness\": " << durezaPincel << L",\n"
          << L"  \"eraserMin\": " << pressaoMinBorracha << L",\n  \"eraserMax\": " << pressaoMaxBorracha << L",\n"
          << L"  \"bucketGap\": " << nivelGapBalde << L",\n  \"bucketTolerance\": " << toleranciaBalde << L",\n"
          << L"  \"lassoMode\": " << modoLaco << L",\n  \"selectionColor\": " << (unsigned long)corSelecao << L",\n"
          << L"  \"theme\": " << temaAtual << L",\n  \"timelineFollowsTheme\": " << (timelineSegueTema?L"true":L"false") << L",\n"
          << L"  \"timelineColor\": " << (unsigned long)corTimelinePersonalizada << L"\n}\n";
    }
}
void CarregarConfigProjeto(const wchar_t* pasta) {
    if(!pasta || !pasta[0]) return;
    lstrcpynW(caminhoProjetoAtivo,pasta,MAX_PATH);
    wchar_t cp[MAX_PATH]; wsprintfW(cp,L"%s\\config.json",pasta);
    std::wifstream cfg(cp); cfg.imbue(std::locale::classic()); if(!cfg.is_open()) return;
    std::wstring d((std::istreambuf_iterator<wchar_t>(cfg)),std::istreambuf_iterator<wchar_t>());
    auto val=[&](const wchar_t* k,int def)->int { std::wstring key=L"\""+std::wstring(k)+L"\""; size_t q=d.find(key); if(q==std::wstring::npos)return def; q=d.find(L':',q+key.size()); if(q==std::wstring::npos)return def; const wchar_t* v=d.c_str()+q+1; while(*v==L' '||*v==L'\t')++v; if(wcsncmp(v,L"true",4)==0)return 1; if(wcsncmp(v,L"false",5)==0)return 0; return _wtoi(v); };
    auto valFloat=[&](const wchar_t* k,double def)->double { std::wstring key=L"\""+std::wstring(k)+L"\""; size_t q=d.find(key); if(q==std::wstring::npos)return def; q=d.find(L':',q+key.size()); if(q==std::wstring::npos)return def; const wchar_t* v=d.c_str()+q+1; while(*v==L' '||*v==L'\t')++v; return wcstod(v,NULL); };
    tamanhoLapis=(float)std::max(0.01,std::min(20000.0,valFloat(L"pencilSize",tamanhoLapis))); tamanhoBorracha=std::max(1,std::min(100,val(L"eraserSize",tamanhoBorracha)));
    pressaoCanetaAtiva=val(L"pencilPressure",pressaoCanetaAtiva?1:0)!=0; pressaoBorrachaAtiva=val(L"eraserPressure",pressaoBorrachaAtiva?1:0)!=0;
    tamanhoMinimoPressao=(float)std::max(0.01,std::min(5000.0,valFloat(L"pencilMin",tamanhoMinimoPressao))); tamanhoMaximoPressao=(float)std::max((double)tamanhoMinimoPressao,std::min(5000.0,valFloat(L"pencilMax",tamanhoMaximoPressao)));
    lapisAntiAlias=val(L"pencilAntiAlias",lapisAntiAlias?1:0)!=0; durezaPincel=std::max(0,std::min(3,val(L"pencilHardness",durezaPincel)));
    pressaoMinBorracha=std::max(1,std::min(100,val(L"eraserMin",pressaoMinBorracha))); pressaoMaxBorracha=std::max(pressaoMinBorracha,std::min(100,val(L"eraserMax",pressaoMaxBorracha)));
    nivelGapBalde=std::max(0,std::min(100,val(L"bucketGap",nivelGapBalde))); toleranciaBalde=std::max(0,std::min(20,val(L"bucketTolerance",toleranciaBalde)));
    modoLaco=val(L"lassoMode",modoLaco)==1?1:0; corSelecao=(COLORREF)val(L"selectionColor",(int)corSelecao);
    temaAtual=std::max(0,std::min(3,val(L"theme",temaAtual))); timelineSegueTema=val(L"timelineFollowsTheme",timelineSegueTema?1:0)!=0;
    corTimelinePersonalizada=(COLORREF)val(L"timelineColor",(int)corTimelinePersonalizada);
    ferramentaSelecionadaID=val(L"toolSelectedId",ferramentaSelecionadaID);
    SalvarConfiguracoesUsuario();
}
COLORREF CorTemaFundo() {
    if (temaAtual == 1) return RGB(238,238,238);
    if (temaAtual == 2) return RGB(255,220,238);
    if (temaAtual == 3) return RGB(20,10,32);
    return RGB(25,25,25);
}
void AplicarCursor(HWND hwnd) {
    if (cursorPersonalizado) SetClassLongPtrW(hwnd, GCLP_HCURSOR, (LONG_PTR)cursorPersonalizado);
    else SetClassLongPtrW(hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursorW(NULL, MAKEINTRESOURCEW(32515)));
    SetCursor(cursorPersonalizado ? cursorPersonalizado : LoadCursorW(NULL, MAKEINTRESOURCEW(32515)));
}
// Aplica afinidade da CPU. 0 significa todos os núcleos disponíveis.
bool AplicarLimiteNucleos(int desejados) {
    DWORD_PTR processMask=0, systemMask=0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask)) return false;
    if (!g_mascaraAfinidadeOriginal) g_mascaraAfinidadeOriginal = processMask;
    if (desejados <= 0) return SetProcessAffinityMask(GetCurrentProcess(), g_mascaraAfinidadeOriginal) != 0;
    DWORD_PTR novo=0; int escolhidos=0;
    for (unsigned int bit=0; bit<sizeof(DWORD_PTR)*8 && escolhidos<desejados; ++bit) {
        DWORD_PTR flag=((DWORD_PTR)1)<<bit;
        if (g_mascaraAfinidadeOriginal & flag) { novo |= flag; ++escolhidos; }
    }
    if (!novo) return false;
    return SetProcessAffinityMask(GetCurrentProcess(), novo) != 0;
}

// Job Object pode impor limite de memória do processo. O limite é rígido:
// se o programa tentar ultrapassá-lo, alocações podem falhar.
bool AplicarLimiteRAM(int mb) {
    if (!hJobLimiteMemoria) {
        hJobLimiteMemoria = CreateJobObjectW(NULL, NULL);
        if (!hJobLimiteMemoria) return false;
        if (!AssignProcessToJobObject(hJobLimiteMemoria, GetCurrentProcess())) {
            CloseHandle(hJobLimiteMemoria); hJobLimiteMemoria=NULL; return false;
        }
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    if (mb > 0) {
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        info.ProcessMemoryLimit = (SIZE_T)mb * 1024u * 1024u;
    }
    return SetInformationJobObject(hJobLimiteMemoria, JobObjectExtendedLimitInformation, &info, sizeof(info)) != 0;
}

void DestruirDirectX9() {
    if (g_dx9CanvasTexture) { g_dx9CanvasTexture->Release(); g_dx9CanvasTexture=NULL; }
    g_dx9TextureWidth = g_dx9TextureHeight = 0;
    if (g_d3dDevice9) { g_d3dDevice9->Release(); g_d3dDevice9=NULL; }
    if (g_d3d9) { g_d3d9->Release(); g_d3d9=NULL; }
}

bool GarantirDirectX9(HWND hwnd) {
    if (g_d3dDevice9) return true;
    g_d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d9) return false;
    D3DPRESENT_PARAMETERS pp{};
    RECT r{}; GetClientRect(hwnd,&r);
    pp.BackBufferWidth=(UINT)std::max<LONG>(1,r.right-r.left);
    pp.BackBufferHeight=(UINT)std::max<LONG>(1,r.bottom-r.top);
    pp.BackBufferFormat=D3DFMT_UNKNOWN;
    pp.BackBufferCount=1; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=hwnd; pp.Windowed=TRUE; pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    HRESULT hr=g_d3d9->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&g_d3dDevice9);
    if (FAILED(hr)) hr=g_d3d9->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hwnd,
        D3DCREATE_MIXED_VERTEXPROCESSING,&pp,&g_d3dDevice9);
    if (FAILED(hr)) hr=g_d3d9->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&g_d3dDevice9);
    if (FAILED(hr)) { DestruirDirectX9(); return false; }
    return true;
}

// Apresenta o backbuffer GDI através de uma textura Direct3D 9. O desenho e as
// ferramentas continuam usando o modelo de bitmap existente, mas a apresentação
// final do canvas passa pelo dispositivo DirectX 9.
bool ApresentarCanvasDirectX9(HWND hwnd, HBITMAP bitmap, int w, int h) {
    if (!GarantirDirectX9(hwnd) || !bitmap || w<=0 || h<=0 || !CopiarCanvasParaStaging(w,h)) return false;
    BYTE* pixels=g_canvasStagingBits;
    HRESULT hr = S_OK;
    if (!g_dx9CanvasTexture || g_dx9TextureWidth != w || g_dx9TextureHeight != h) {
        if (g_dx9CanvasTexture) { g_dx9CanvasTexture->Release(); g_dx9CanvasTexture=NULL; }
        g_dx9TextureWidth = g_dx9TextureHeight = 0;
        hr = g_d3dDevice9->CreateTexture((UINT)w,(UINT)h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&g_dx9CanvasTexture,NULL);
        if (SUCCEEDED(hr)) { g_dx9TextureWidth=w; g_dx9TextureHeight=h; }
    }
    if(SUCCEEDED(hr) && g_dx9CanvasTexture) {
        D3DLOCKED_RECT lr{}; hr=g_dx9CanvasTexture->LockRect(0,&lr,NULL,0);
        if(SUCCEEDED(hr)) {
            for(int y=0;y<h;++y) CopyMemory((BYTE*)lr.pBits+(size_t)y*lr.Pitch,pixels+(size_t)y*w*4,(size_t)w*4);
            g_dx9CanvasTexture->UnlockRect(0);
            struct V { float x,y,z,rhw,u,v; };
            RECT rc{}; GetClientRect(hwnd,&rc); float cw=(float)std::max<LONG>(1,rc.right), ch=(float)std::max<LONG>(1,rc.bottom);
            V quad[4]={{-0.5f,-0.5f,0,1,0,0},{cw-0.5f,-0.5f,0,1,1,0},{-0.5f,ch-0.5f,0,1,0,1},{cw-0.5f,ch-0.5f,0,1,1,1}};
            g_d3dDevice9->SetRenderState(D3DRS_LIGHTING,FALSE); g_d3dDevice9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
            g_d3dDevice9->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
            g_d3dDevice9->SetTexture(0,g_dx9CanvasTexture); g_d3dDevice9->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
            g_d3dDevice9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR); g_d3dDevice9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
            g_d3dDevice9->Clear(0,NULL,D3DCLEAR_TARGET,D3DCOLOR_XRGB(25,25,25),1.0f,0);
            if(SUCCEEDED(g_d3dDevice9->BeginScene())) { g_d3dDevice9->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(V)); g_d3dDevice9->EndScene(); }
            hr=g_d3dDevice9->Present(NULL,NULL,NULL,NULL);
        }
    }
    return SUCCEEDED(hr);
}

int DetectarFamiliaWindows();

typedef HRESULT (WINAPI *PFN_D3D11CreateDeviceAndSwapChain)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,UINT,UINT,const DXGI_SWAP_CHAIN_DESC*,IDXGISwapChain**,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
typedef HRESULT (WINAPI *PFN_CreateDXGIFactory1)(REFIID,void**);
typedef HRESULT (WINAPI *PFN_D3D12CreateDevice)(IUnknown*,D3D_FEATURE_LEVEL,REFIID,void**);

void DestruirDirectX11(){ if(g_dx11Texture){g_dx11Texture->Release();g_dx11Texture=NULL;} if(g_dx11Swap){g_dx11Swap->Release();g_dx11Swap=NULL;} if(g_d3d11Context){g_d3d11Context->ClearState();g_d3d11Context->Release();g_d3d11Context=NULL;} if(g_d3d11Device){g_d3d11Device->Release();g_d3d11Device=NULL;} g_dx11W=g_dx11H=0; }
bool GarantirDirectX11(HWND hwnd,int w,int h){
    if(g_dx11Swap && g_dx11W==w && g_dx11H==h) return true;
    if(!g_modD3D11) g_modD3D11=LoadLibraryW(L"d3d11.dll"); if(!g_modDXGI) g_modDXGI=LoadLibraryW(L"dxgi.dll");
    if(!g_modD3D11||!g_modDXGI) return false;
    PFN_D3D11CreateDeviceAndSwapChain create=(PFN_D3D11CreateDeviceAndSwapChain)GetProcAddress(g_modD3D11,"D3D11CreateDeviceAndSwapChain"); if(!create)return false;
    if(g_dx11Swap){ DestruirDirectX11(); }
    DXGI_SWAP_CHAIN_DESC sd{}; sd.BufferDesc.Width=(UINT)w; sd.BufferDesc.Height=(UINT)h; sd.BufferDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; sd.SampleDesc.Count=1; sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.BufferCount=2; sd.OutputWindow=hwnd; sd.Windowed=TRUE; sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0}; D3D_FEATURE_LEVEL got{};
    HRESULT hr=create(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,0,levels,3,D3D11_SDK_VERSION,&sd,&g_dx11Swap,&g_d3d11Device,&got,&g_d3d11Context);
    if(FAILED(hr)) hr=create(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,levels,3,D3D11_SDK_VERSION,&sd,&g_dx11Swap,&g_d3d11Device,&got,&g_d3d11Context);
    if(FAILED(hr)){DestruirDirectX11();return false;}
    D3D11_TEXTURE2D_DESC td{}; td.Width=(UINT)w; td.Height=(UINT)h; td.MipLevels=1; td.ArraySize=1; td.Format=DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc.Count=1; td.Usage=D3D11_USAGE_DEFAULT; td.BindFlags=0;
    hr=g_d3d11Device->CreateTexture2D(&td,NULL,&g_dx11Texture); if(FAILED(hr)){DestruirDirectX11();return false;} g_dx11W=w;g_dx11H=h;return true;
}
bool ApresentarCanvasDirectX11(HWND hwnd,int w,int h){
    if(!GarantirDirectX11(hwnd,w,h)||!CopiarCanvasParaStaging(w,h))return false;
    // O DIB de staging já contém BGRA contíguo; entrega-o diretamente ao runtime.
    g_d3d11Context->UpdateSubresource(g_dx11Texture,0,NULL,g_canvasStagingBits,(UINT)w*4,0);
    ID3D11Texture2D* back=NULL; HRESULT hr=g_dx11Swap->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back);
    if(SUCCEEDED(hr)&&back){g_d3d11Context->CopyResource(back,g_dx11Texture);back->Release();hr=g_dx11Swap->Present(0,0);} return SUCCEEDED(hr);
}

void EsperarDirectX12(){ if(g_d3d12Queue&&g_dx12Fence&&g_dx12FenceEvent){UINT64 v=++g_dx12FenceValue;if(SUCCEEDED(g_d3d12Queue->Signal(g_dx12Fence,v))&&g_dx12Fence->GetCompletedValue()<v){g_dx12Fence->SetEventOnCompletion(v,g_dx12FenceEvent);WaitForSingleObject(g_dx12FenceEvent,INFINITE);}} }
void DestruirDirectX12(){ EsperarDirectX12(); if(g_dx12Upload){g_dx12Upload->Release();g_dx12Upload=NULL;} if(g_dx12List){g_dx12List->Release();g_dx12List=NULL;} if(g_dx12Allocator){g_dx12Allocator->Release();g_dx12Allocator=NULL;} if(g_dx12Swap){g_dx12Swap->Release();g_dx12Swap=NULL;} if(g_dx12Fence){g_dx12Fence->Release();g_dx12Fence=NULL;} if(g_d3d12Queue){g_d3d12Queue->Release();g_d3d12Queue=NULL;} if(g_d3d12Device){g_d3d12Device->Release();g_d3d12Device=NULL;} if(g_dx12FenceEvent){CloseHandle(g_dx12FenceEvent);g_dx12FenceEvent=NULL;} g_dx12UploadSize=0;g_dx12W=g_dx12H=0;g_dx12FenceValue=0; }
bool GarantirDirectX12(HWND hwnd,int w,int h){
    if(g_dx12Swap&&g_dx12W==w&&g_dx12H==h)return true;
    if(!g_modD3D12)g_modD3D12=LoadLibraryW(L"d3d12.dll"); if(!g_modDXGI)g_modDXGI=LoadLibraryW(L"dxgi.dll"); if(!g_modD3D12||!g_modDXGI)return false;
    PFN_D3D12CreateDevice createDev=(PFN_D3D12CreateDevice)GetProcAddress(g_modD3D12,"D3D12CreateDevice"); PFN_CreateDXGIFactory1 createFactory=(PFN_CreateDXGIFactory1)GetProcAddress(g_modDXGI,"CreateDXGIFactory1"); if(!createDev||!createFactory)return false;
    if(g_dx12Swap)DestruirDirectX12();
    HRESULT hr=createDev(NULL,D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),(void**)&g_d3d12Device); if(FAILED(hr)){DestruirDirectX12();return false;}
    D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;hr=g_d3d12Device->CreateCommandQueue(&qd,__uuidof(ID3D12CommandQueue),(void**)&g_d3d12Queue); if(FAILED(hr)){DestruirDirectX12();return false;}
    IDXGIFactory1* f1=NULL; IDXGIFactory2* f2=NULL; hr=createFactory(__uuidof(IDXGIFactory1),(void**)&f1); if(SUCCEEDED(hr)&&f1)hr=f1->QueryInterface(__uuidof(IDXGIFactory2),(void**)&f2); if(f1)f1->Release(); if(FAILED(hr)||!f2){if(f2)f2->Release();DestruirDirectX12();return false;}
    DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=(UINT)w;sd.Height=(UINT)h;sd.Format=DXGI_FORMAT_B8G8R8A8_UNORM;sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    IDXGISwapChain1* s1=NULL;hr=f2->CreateSwapChainForHwnd(g_d3d12Queue,hwnd,&sd,NULL,NULL,&s1);f2->Release();if(SUCCEEDED(hr)&&s1){hr=s1->QueryInterface(__uuidof(IDXGISwapChain3),(void**)&g_dx12Swap);s1->Release();}
    if(FAILED(hr)||!g_dx12Swap){DestruirDirectX12();return false;}
    hr=g_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,__uuidof(ID3D12CommandAllocator),(void**)&g_dx12Allocator);if(FAILED(hr)){DestruirDirectX12();return false;}
    hr=g_d3d12Device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,g_dx12Allocator,NULL,__uuidof(ID3D12GraphicsCommandList),(void**)&g_dx12List);if(FAILED(hr)){DestruirDirectX12();return false;}g_dx12List->Close();
    hr=g_d3d12Device->CreateFence(0,D3D12_FENCE_FLAG_NONE,__uuidof(ID3D12Fence),(void**)&g_dx12Fence);if(FAILED(hr)){DestruirDirectX12();return false;}g_dx12FenceEvent=CreateEventW(NULL,FALSE,FALSE,NULL);if(!g_dx12FenceEvent){DestruirDirectX12();return false;}g_dx12W=w;g_dx12H=h;return true;
}
bool ApresentarCanvasDirectX12(HWND hwnd,int w,int h){
    if(!GarantirDirectX12(hwnd,w,h)||!CopiarCanvasParaStaging(w,h))return false;
    UINT index=g_dx12Swap->GetCurrentBackBufferIndex();ID3D12Resource* back=NULL;HRESULT hr=g_dx12Swap->GetBuffer(index,__uuidof(ID3D12Resource),(void**)&back);if(FAILED(hr)||!back)return false;
    D3D12_RESOURCE_DESC bd=back->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows=0;UINT64 rowBytes=0,total=0;g_d3d12Device->GetCopyableFootprints(&bd,0,1,0,&fp,&rows,&rowBytes,&total);
    if(!g_dx12Upload||g_dx12UploadSize<total){if(g_dx12Upload){g_dx12Upload->Release();g_dx12Upload=NULL;}D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=total;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;hr=g_d3d12Device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,NULL,__uuidof(ID3D12Resource),(void**)&g_dx12Upload);if(FAILED(hr)){back->Release();return false;}g_dx12UploadSize=total;}
    BYTE* mapped=NULL;D3D12_RANGE readRange{0,0};hr=g_dx12Upload->Map(0,&readRange,(void**)&mapped);if(FAILED(hr)||!mapped){back->Release();return false;}
    for(int y=0;y<h;++y)CopyMemory(mapped+fp.Offset+(size_t)y*fp.Footprint.RowPitch,g_canvasStagingBits+(size_t)y*w*4,(size_t)w*4);g_dx12Upload->Unmap(0,NULL);
    EsperarDirectX12();hr=g_dx12Allocator->Reset();if(SUCCEEDED(hr))hr=g_dx12List->Reset(g_dx12Allocator,NULL);
    if(SUCCEEDED(hr)){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=back;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;g_dx12List->ResourceBarrier(1,&b);D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=back;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=0;D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=g_dx12Upload;src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp;g_dx12List->CopyTextureRegion(&dst,0,0,0,&src,NULL);b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;g_dx12List->ResourceBarrier(1,&b);hr=g_dx12List->Close();}
    if(SUCCEEDED(hr)){ID3D12CommandList* lists[]={g_dx12List};g_d3d12Queue->ExecuteCommandLists(1,lists);hr=g_dx12Swap->Present(0,0);}back->Release();return SUCCEEDED(hr);
}
bool VerificarDirectX11Disponivel(){
    HMODULE m=LoadLibraryW(L"d3d11.dll"); if(!m)return false;
    PFN_D3D11CreateDeviceAndSwapChain f=(PFN_D3D11CreateDeviceAndSwapChain)GetProcAddress(m,"D3D11CreateDeviceAndSwapChain");
    bool ok=false; if(f){ID3D11Device* d=NULL;ID3D11DeviceContext* c=NULL;D3D_FEATURE_LEVEL got{};D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
        HRESULT hr=f(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,0,levels,3,D3D11_SDK_VERSION,NULL,NULL,&d,&got,&c);
        ok=SUCCEEDED(hr);if(c)c->Release();if(d)d->Release();}
    FreeLibrary(m);return ok;
}
bool VerificarDirectX12Disponivel(){if(DetectarFamiliaWindows()<2)return false;HMODULE m=LoadLibraryW(L"d3d12.dll");if(!m)return false;PFN_D3D12CreateDevice f=(PFN_D3D12CreateDevice)GetProcAddress(m,"D3D12CreateDevice");bool ok=false;if(f){ID3D12Device* d=NULL;ok=SUCCEEDED(f(NULL,D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),(void**)&d));if(d)d->Release();}FreeLibrary(m);return ok;}
// Primeira execução: escolhe a API moderna que conseguiu inicializar em GPU,
// respeitando a compatibilidade do sistema; a escolha do usuário fica salva.
static int DetectarMelhorMotorGrafico() {
    const int os=DetectarFamiliaWindows();
    if(os>=2 && VerificarDirectX12Disponivel()) return 3;
    if(os>=1 && VerificarDirectX11Disponivel()) return 2;
    IDirect3D9* test=Direct3DCreate9(D3D_SDK_VERSION);
    if(test) { test->Release(); return 1; }
    return 0;
}

// Detecta a versão real do Windows sem depender do manifesto do executável.
// Retorno: 0=XP/Vista/mais antigo, 1=Windows 7 a 8.1, 2=Windows 10/11.
struct KodaOSVersionInfo { DWORD dwOSVersionInfoSize; DWORD dwMajorVersion; DWORD dwMinorVersion; DWORD dwBuildNumber; DWORD dwPlatformId; WCHAR szCSDVersion[128]; };
int DetectarFamiliaWindows() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return 0;
    typedef LONG (WINAPI *RtlGetVersionFn)(KodaOSVersionInfo*);
    RtlGetVersionFn fn = (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion");
    if (!fn) return 0;
    KodaOSVersionInfo v{}; v.dwOSVersionInfoSize = sizeof(v);
    if (fn(&v) != 0) return 0;
    if (v.dwMajorVersion >= 10) return 2; // Windows 10/11
    if (v.dwMajorVersion == 6 && v.dwMinorVersion >= 1 && v.dwMinorVersion <= 3) return 1; // Windows 7/8/8.1
    return 0; // XP e sistemas anteriores a Windows 7: GDI/DirectX 9
}
int MaxMotorPorWindows() {
    int familia = DetectarFamiliaWindows();
    return familia >= 2 ? 3 : familia == 1 ? 2 : 1;
}
const wchar_t* NomeFamiliaWindows() {
    int familia = DetectarFamiliaWindows();
    return familia >= 2 ? L"Windows 10/11" : familia == 1 ? L"Windows 7/8/8.1" : L"Windows XP/Vista ou anterior";
}

void AbrirConfiguracoesUsuario(HWND owner) {
    // O editor de atalhos agora faz parte da aba Atalhos; não manter a janela antiga separada.
    if (hwndShortcutPanel && IsWindow(hwndShortcutPanel)) { DestroyWindow(hwndShortcutPanel); hwndShortcutPanel = NULL; }
    if (hwndSettings && IsWindow(hwndSettings)) {
        SetWindowPos(hwndSettings, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(hwndSettings, SW_SHOW);
        SetForegroundWindow(hwndSettings);
        return;
    }

    WNDCLASSEXW swc{};
    swc.cbSize = sizeof(swc);
    swc.lpfnWndProc = WndProcSettings;
    swc.hInstance = hInstGlobal;
    swc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    swc.hIcon = hIconeApp;
    swc.hIconSm = hIconeApp;
    swc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    swc.lpszClassName = L"KodanimaSettingsClass";
    RegisterClassExW(&swc);

    settingsMouseEntered = false;
    hwndSettings = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"KodanimaSettingsClass",
        L"Preferências do Kodanima",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 880, 650, owner, NULL, hInstGlobal, NULL);
    if (!hwndSettings) return;

    // Estrutura inspirada no OpenToonz: categorias fixas à esquerda e uma
    // única página de opções à direita. Cada controle tem ID próprio; a
    // visibilidade nunca depende da coordenada em que o controle foi criado.
    CreateWindowW(L"STATIC", L"PREFERÊNCIAS", WS_VISIBLE | WS_CHILD,
                  15, 14, 145, 22, hwndSettings,
                  (HMENU)(INT_PTR)ID_SETTINGS_HEADER, hInstGlobal, NULL);
    HWND nav = CreateWindowW(L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        15, 42, 145, 475, hwndSettings, (HMENU)(INT_PTR)469, hInstGlobal, NULL);
    const wchar_t* cats[] = {
        L"Interface", L"Gráfico", L"Tablet", L"Timeline", L"Histórico", L"Atalhos", L"Explicação"
    };
    for (int i = 0; i < 7; ++i) {
        std::wstring catLabel=(i==6)?KodaRotuloExplicacao(false):KodaTraduzirTexto(cats[i]);
        SendMessageW(nav, LB_ADDSTRING, 0, (LPARAM)catLabel.c_str());
    }
    SendMessageW(nav, LB_SETCURSEL, 0, 0);

    CreateWindowW(L"STATIC", L"Interface", WS_VISIBLE | WS_CHILD,
        182, 14, 660, 26, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_CATEGORY_TITLE, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"Selecione uma função e escolha a nova tecla. As alterações ficam ativas após clicar em Aplicar.",
        WS_CHILD, 195, 66, 620, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_SHORTCUT_NOTE, hInstGlobal, NULL);

    // Atalhos dentro da própria página, sem janela flutuante separada.
    CreateWindowW(L"STATIC", L"Funções e teclas", WS_CHILD,
        195, 104, 400, 24, hwndSettings, NULL, hInstGlobal, NULL);
    CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_BORDER | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
        195, 130, 405, 345, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_LIST, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"Nova tecla", WS_CHILD,
        620, 130, 180, 24, hwndSettings, NULL, hInstGlobal, NULL);
    HWND hotkeyCombo = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        620, 158, 190, 220, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_KEY, hInstGlobal, NULL);
    const wchar_t* shortcutKeys[] = {
        L",", L".", L"Enter", L"Tab", L"Delete", L"F1", L"F2", L"F3", L"F4", L"F5",
        L"F6", L"F7", L"F8", L"F9", L"F10", L"F11", L"F12",
        L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L",
        L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z",
        L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
        L"Home", L"End", L"PageUp", L"PageDown", L"Num +", L"Num -", L"Num 4", L"Num 6"
    };
    for (int i = 0; i < (int)(sizeof(shortcutKeys) / sizeof(shortcutKeys[0])); ++i) {
        int n = (int)SendMessageW(hotkeyCombo, CB_ADDSTRING, 0, (LPARAM)shortcutKeys[i]);
        SendMessageW(hotkeyCombo, CB_SETITEMDATA, n, VkPorIndiceTeclaKoda(i));
    }
    CreateWindowW(L"BUTTON", L"Aplicar tecla", WS_CHILD,
        620, 198, 190, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_APPLY, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"Salvar atalhos", WS_CHILD,
        620, 242, 190, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_SAVE, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"Restaurar padrão", WS_CHILD,
        620, 286, 190, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_RESET, hInstGlobal, NULL);

    // Guia integrado em texto, somente leitura e com rolagem vertical.
    HWND explanation=CreateWindowW(L"EDIT",L"",
        WS_CHILD|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|ES_NOHIDESEL|WS_TABSTOP,
        195,56,625,470,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_EXPLANATION_TEXT,hInstGlobal,NULL);
    if(explanation) {
        SendMessageW(explanation,EM_SETLIMITTEXT,60000,0);
        const std::wstring guide=KodaMontarTextoExplicacao();
        SetWindowTextW(explanation,guide.c_str());
    }

    // Interface
    CreateWindowW(L"STATIC", L"Tema do programa", WS_VISIBLE | WS_CHILD,
        195, 66, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_THEME, hInstGlobal, NULL);
    hwndComboTema = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 61, 300, 140, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_THEME, hInstGlobal, NULL);
    SendMessageW(hwndComboTema, CB_ADDSTRING, 0, (LPARAM)L"Escuro");
    SendMessageW(hwndComboTema, CB_ADDSTRING, 0, (LPARAM)L"Claro");
    SendMessageW(hwndComboTema, CB_ADDSTRING, 0, (LPARAM)L"Pink");
    SendMessageW(hwndComboTema, CB_ADDSTRING, 0, (LPARAM)L"Black com roxo");
    SendMessageW(hwndComboTema, CB_SETCURSEL, temaAtual, 0);

    CreateWindowW(L"STATIC", L"Tamanho da interface", WS_VISIBLE | WS_CHILD,
        195, 119, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_UI_SCALE, hInstGlobal, NULL);
    hwndComboUIScale = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 114, 300, 120, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_UI_SCALE, hInstGlobal, NULL);
    SendMessageW(hwndComboUIScale, CB_ADDSTRING, 0, (LPARAM)L"80%");
    SendMessageW(hwndComboUIScale, CB_ADDSTRING, 0, (LPARAM)L"100%");
    SendMessageW(hwndComboUIScale, CB_ADDSTRING, 0, (LPARAM)L"125%");
    SendMessageW(hwndComboUIScale, CB_ADDSTRING, 0, (LPARAM)L"150%");
    SendMessageW(hwndComboUIScale, CB_SETCURSEL,
        escalaInterface == 80 ? 0 : escalaInterface == 125 ? 2 : escalaInterface == 150 ? 3 : 1, 0);

    CreateWindowW(L"STATIC", L"Tamanho do texto", WS_VISIBLE | WS_CHILD,
        195, 172, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_TEXT_SIZE, hInstGlobal, NULL);
    hwndComboTextSize = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 167, 300, 120, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_TEXT_SIZE, hInstGlobal, NULL);
    SendMessageW(hwndComboTextSize, CB_ADDSTRING, 0, (LPARAM)L"80%");
    SendMessageW(hwndComboTextSize, CB_ADDSTRING, 0, (LPARAM)L"100%");
    SendMessageW(hwndComboTextSize, CB_ADDSTRING, 0, (LPARAM)L"125%");
    SendMessageW(hwndComboTextSize, CB_ADDSTRING, 0, (LPARAM)L"150%");
    SendMessageW(hwndComboTextSize, CB_SETCURSEL,
        tamanhoTexto == 80 ? 0 : tamanhoTexto == 125 ? 2 : tamanhoTexto == 150 ? 3 : 1, 0);

    // Gráfico e desempenho
    CreateWindowW(L"STATIC", L"Motor gráfico preferido", WS_VISIBLE | WS_CHILD,
        195, 66, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_RENDERER, hInstGlobal, NULL);
    hwndComboRenderer = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 61, 300, 145, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_RENDERER, hInstGlobal, NULL);
    auto adicionarMotor = [&](const wchar_t* nome, int id) {
        int i = (int)SendMessageW(hwndComboRenderer, CB_ADDSTRING, 0, (LPARAM)nome);
        SendMessageW(hwndComboRenderer, CB_SETITEMDATA, i, id);
    };
    adicionarMotor(L"GDI nativo", 0);
    adicionarMotor(L"DirectX 9", 1);
    int familiaDX = DetectarFamiliaWindows();
    bool dx11Disponivel = familiaDX >= 1 && VerificarDirectX11Disponivel();
    bool dx12Disponivel = familiaDX >= 2 && VerificarDirectX12Disponivel();
    if (dx11Disponivel || rendererPreferido == 2)
        adicionarMotor(dx11Disponivel ? L"DirectX 11" : L"DirectX 11 (indisponível neste sistema)", 2);
    if (dx12Disponivel || rendererPreferido == 3)
        adicionarMotor(dx12Disponivel ? L"DirectX 12" : L"DirectX 12 (indisponível neste sistema)", 3);
    int idxMotor = 0;
    for (int i = 0; i < (int)SendMessageW(hwndComboRenderer, CB_GETCOUNT, 0, 0); ++i)
        if ((int)SendMessageW(hwndComboRenderer, CB_GETITEMDATA, i, 0) == rendererPreferido)
            idxMotor = i;
    SendMessageW(hwndComboRenderer, CB_SETCURSEL, idxMotor, 0);

    CreateWindowW(L"STATIC", L"Núcleos de CPU (0 = todos)", WS_VISIBLE | WS_CHILD,
        195, 119, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_CORES, hInstGlobal, NULL);
    hwndComboCores = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 114, 300, 150, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_CORES, hInstGlobal, NULL);
    SendMessageW(hwndComboCores, CB_ADDSTRING, 0, (LPARAM)L"Todos os núcleos");
    SYSTEM_INFO si{}; GetSystemInfo(&si);
    DWORD maxCores = std::max<DWORD>(1, si.dwNumberOfProcessors);
    for (DWORD c = 1; c <= maxCores && c <= 64; ++c) {
        wchar_t label[48]; wsprintfW(label, L"%lu núcleo(s)", (unsigned long)c);
        SendMessageW(hwndComboCores, CB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageW(hwndComboCores, CB_SETCURSEL,
        nucleosDesejados > 0 && nucleosDesejados <= (int)maxCores ? nucleosDesejados : 0, 0);

    CreateWindowW(L"STATIC", L"Limite de RAM", WS_VISIBLE | WS_CHILD,
        195, 172, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_RAM, hInstGlobal, NULL);
    hwndComboRAM = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 167, 300, 150, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_RAM, hInstGlobal, NULL);
    const wchar_t* ramOpts[] = {
        L"Sem limite", L"512 MB", L"1024 MB (1 GB)",
        L"2048 MB (2 GB)", L"4096 MB (4 GB)", L"8192 MB (8 GB)"
    };
    for (int i = 0; i < 6; ++i) SendMessageW(hwndComboRAM, CB_ADDSTRING, 0, (LPARAM)ramOpts[i]);
    int ramSel = limiteRAMMB == 512 ? 1 : limiteRAMMB == 1024 ? 2 : limiteRAMMB == 2048 ? 3 : limiteRAMMB == 4096 ? 4 : limiteRAMMB == 8192 ? 5 : 0;
    SendMessageW(hwndComboRAM, CB_SETCURSEL, ramSel, 0);
    std::wstring infoSistema = L"Sistema detectado: ";
    infoSistema += NomeFamiliaWindows();
    CreateWindowW(L"STATIC", infoSistema.c_str(), WS_VISIBLE | WS_CHILD,
        195, 231, 540, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_SYSTEM, hInstGlobal, NULL);

    CreateWindowW(L"STATIC",L"CPU: aguardando detecção...",WS_VISIBLE|WS_CHILD,
        195,258,625,19,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_CPU_INFO,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Recursos: --",WS_VISIBLE|WS_CHILD,
        195,280,625,19,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_CPU_COUNT,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Kernel de pixels: --",WS_VISIBLE|WS_CHILD,
        195,302,625,19,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_CPU_MODE,hInstGlobal,NULL);
    const struct { int id; const wchar_t* text; int x; int y; int w; } cpuRows[]={
        {ID_SETTINGS_CPU_MMX,L"MMX: --",195,327,290},
        {ID_SETTINGS_CPU_SSE,L"SSE: --",195,349,290},
        {ID_SETTINGS_CPU_SSE2,L"SSE2: --",195,371,290},
        {ID_SETTINGS_CPU_SSE3,L"SSE3: --",195,393,290},
        {ID_SETTINGS_CPU_SSSE3,L"SSSE3: --",195,415,290},
        {ID_SETTINGS_CPU_SSE41,L"SSE4.1: --",195,437,290},
        {ID_SETTINGS_CPU_SSE42,L"SSE4.2: --",500,327,315},
        {ID_SETTINGS_CPU_AVX,L"AVX: --",500,349,315},
        {ID_SETTINGS_CPU_AVX2,L"AVX2: --",500,371,315},
        {ID_SETTINGS_CPU_AES,L"AES: --",500,393,315},
        {ID_SETTINGS_CPU_VIRTUAL,L"VT-x / AMD-V: --",500,415,315}
    };
    for(size_t ci=0;ci<sizeof(cpuRows)/sizeof(cpuRows[0]);++ci)
        CreateWindowW(L"STATIC",cpuRows[ci].text,WS_VISIBLE|WS_CHILD,
            cpuRows[ci].x,cpuRows[ci].y,cpuRows[ci].w,19,hwndSettings,
            (HMENU)(INT_PTR)cpuRows[ci].id,hInstGlobal,NULL);

    // Tablet
    CreateWindowW(L"STATIC", L"API de pressão da caneta", WS_VISIBLE | WS_CHILD,
        195, 66, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_PRESSURE_API, hInstGlobal, NULL);
    hwndComboPressureAPI = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 61, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_PRESSURE_API, hInstGlobal, NULL);
    SendMessageW(hwndComboPressureAPI, CB_ADDSTRING, 0, (LPARAM)L"Wintab (Wintab32.dll)");
    SendMessageW(hwndComboPressureAPI, CB_ADDSTRING, 0, (LPARAM)L"Windows Pointer / Ink (Windows 8+)");
    SendMessageW(hwndComboPressureAPI, CB_SETCURSEL, pressureApi, 0);
    CreateWindowW(L"BUTTON", L"Escolher cursor .cur/.ani",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        195, 116, 220, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_CURSOR, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"A API selecionada é usada para entrada da caneta digitalizadora.",
        WS_VISIBLE | WS_CHILD, 195, 164, 540, 26, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_TABLET_NOTE, hInstGlobal, NULL);

    CreateWindowW(L"STATIC",L"Mesa detectada:",WS_VISIBLE|WS_CHILD,195,204,220,22,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LABEL_DEVICE,hInstGlobal,NULL);
    g_tabletDeviceValueHwnd=CreateWindowW(L"STATIC",g_tabletDetectedName.c_str(),WS_VISIBLE|WS_CHILD,435,201,385,28,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_DEVICE_VALUE,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Driver / versão:",WS_VISIBLE|WS_CHILD,195,235,220,22,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LABEL_DRIVER,hInstGlobal,NULL);
    g_tabletDriverValueHwnd=CreateWindowW(L"STATIC",L"Consultando driver...",WS_VISIBLE|WS_CHILD,435,232,385,28,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_DRIVER_VALUE,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Níveis de pressão:",WS_VISIBLE|WS_CHILD,195,266,220,22,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LABEL_LEVELS,hInstGlobal,NULL);
    g_tabletLevelsValueHwnd=CreateWindowW(L"STATIC",L"Consultando eixo de pressão...",WS_VISIBLE|WS_CHILD,435,263,385,38,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LEVELS_VALUE,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Nome manual do perfil:",WS_VISIBLE|WS_CHILD,195,312,220,22,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LABEL_MANUAL_NAME,hInstGlobal,NULL);
    HWND tabletNameEdit=CreateWindowW(L"EDIT",g_tabletManualDeviceName.c_str(),WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,435,307,300,25,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_NAME_EDIT,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Níveis manuais (0 = automático):",WS_VISIBLE|WS_CHILD,195,347,230,22,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LABEL_MANUAL_LEVELS,hInstGlobal,NULL);
    wchar_t tabletLevelsText[32]; wsprintfW(tabletLevelsText,L"%d",g_tabletManualPressureLevels);
    CreateWindowW(L"EDIT",tabletLevelsText,WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,435,342,100,25,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_LEVELS_EDIT,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Aplicar perfil",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,550,340,145,29,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_APPLY,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"O eixo Wintab pode informar o intervalo físico real. Windows Pointer/Ink entrega pressão normalizada 0–1024; o perfil por fabricante é estimado, não uma leitura exata do hardware.",WS_VISIBLE|WS_CHILD,195,386,625,54,hwndSettings,(HMENU)(INT_PTR)ID_SETTINGS_TABLET_NOTE,hInstGlobal,NULL);
    AtualizarPainelTablet();

    // Timeline
    CreateWindowW(L"STATIC", L"Tamanho dos botões da timeline", WS_VISIBLE | WS_CHILD,
        195, 66, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_TIMELINE_BUTTONS, hInstGlobal, NULL);
    HWND comboTimelineButtons = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 61, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_TIMELINE_BUTTONS, hInstGlobal, NULL);
    SendMessageW(comboTimelineButtons, CB_ADDSTRING, 0, (LPARAM)L"70% - compacto");
    SendMessageW(comboTimelineButtons, CB_ADDSTRING, 0, (LPARAM)L"80% - pequeno");
    SendMessageW(comboTimelineButtons, CB_ADDSTRING, 0, (LPARAM)L"90% - médio");
    SendMessageW(comboTimelineButtons, CB_ADDSTRING, 0, (LPARAM)L"100% - normal");
    SendMessageW(comboTimelineButtons, CB_SETCURSEL,
        tamanhoBotoesTimeline == 70 ? 0 : tamanhoBotoesTimeline == 90 ? 2 : tamanhoBotoesTimeline == 100 ? 3 : 1, 0);

    CreateWindowW(L"STATIC", L"Altura das camadas", WS_VISIBLE | WS_CHILD,
        195, 119, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_LAYER_HEIGHT, hInstGlobal, NULL);
    HWND comboLayerHeight = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 114, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LAYER_HEIGHT, hInstGlobal, NULL);
    const wchar_t* layerHeightOpts[] = {
        L"22 px - compacta", L"27 px - normal", L"32 px - grande", L"38 px - extra grande"
    };
    for (int i = 0; i < 4; ++i) SendMessageW(comboLayerHeight, CB_ADDSTRING, 0, (LPARAM)layerHeightOpts[i]);
    SendMessageW(comboLayerHeight, CB_SETCURSEL,
        alturaLinhaCamada == 22 ? 0 : alturaLinhaCamada == 32 ? 2 : alturaLinhaCamada == 38 ? 3 : 1, 0);

    CreateWindowW(L"STATIC", L"Altura da régua", WS_VISIBLE | WS_CHILD,
        195, 172, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_RULER_HEIGHT, hInstGlobal, NULL);
    HWND comboRulerHeight = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 167, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_RULER_HEIGHT, hInstGlobal, NULL);
    const wchar_t* rulerHeightOpts[] = {
        L"18 px - compacta", L"24 px - normal", L"30 px - grande", L"36 px - extra grande"
    };
    for (int i = 0; i < 4; ++i) SendMessageW(comboRulerHeight, CB_ADDSTRING, 0, (LPARAM)rulerHeightOpts[i]);
    SendMessageW(comboRulerHeight, CB_SETCURSEL,
        alturaReguaTimeline == 18 ? 0 : alturaReguaTimeline == 30 ? 2 : alturaReguaTimeline == 36 ? 3 : 1, 0);

    // Histórico
    CreateWindowW(L"STATIC", L"Desfazer / refazer", WS_VISIBLE | WS_CHILD,
        195, 66, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_HISTORY_ACTIONS, hInstGlobal, NULL);
    HWND comboHistoryActions = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 61, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HISTORY_ACTIONS, hInstGlobal, NULL);
    const wchar_t* historyActionsOpts[] = {
        L"5 ações", L"10 ações", L"20 ações (padrão)", L"30 ações", L"50 ações"
    };
    for (int i = 0; i < 5; ++i) SendMessageW(comboHistoryActions, CB_ADDSTRING, 0, (LPARAM)historyActionsOpts[i]);
    SendMessageW(comboHistoryActions, CB_SETCURSEL,
        historyActionLimit == 5 ? 0 : historyActionLimit == 10 ? 1 : historyActionLimit == 30 ? 3 : historyActionLimit == 50 ? 4 : 2, 0);

    CreateWindowW(L"STATIC", L"Limite de cache para o histórico", WS_VISIBLE | WS_CHILD,
        195, 119, 220, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_HISTORY_CACHE, hInstGlobal, NULL);
    HWND comboHistoryCache = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        435, 114, 300, 130, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_HISTORY_CACHE, hInstGlobal, NULL);
    const wchar_t* historyCacheOpts[] = {
        L"16 MB", L"32 MB", L"64 MB", L"128 MB", L"200 MB (máximo)"
    };
    for (int i = 0; i < 5; ++i) SendMessageW(comboHistoryCache, CB_ADDSTRING, 0, (LPARAM)historyCacheOpts[i]);
    SendMessageW(comboHistoryCache, CB_SETCURSEL,
        historyCacheLimitMB == 16 ? 0 : historyCacheLimitMB == 32 ? 1 : historyCacheLimitMB == 64 ? 2 : historyCacheLimitMB == 128 ? 3 : 4, 0);
    CreateWindowW(L"STATIC", L"O cache limita a memória usada para desfazer e refazer ações.",
        WS_VISIBLE | WS_CHILD, 195, 166, 540, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_HISTORY_NOTE, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"Configuração salva em Documentos\\Kodanima\\config.ini",
        WS_VISIBLE | WS_CHILD, 195, 198, 540, 24, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_LABEL_CONFIG_PATH, hInstGlobal, NULL);

    CreateWindowW(L"BUTTON", L"Salvar configurações",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
        182, 545, 180, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_SAVE, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"Redefinir",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        375, 545, 110, 32, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_RESET, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"GPT C++",
        WS_VISIBLE | WS_CHILD | SS_RIGHT, 630, 562, 200, 18, hwndSettings,
        NULL, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"By creator Gabriel Freitas",
        WS_VISIBLE | WS_CHILD | SS_RIGHT, 630, 582, 200, 18, hwndSettings,
        (HMENU)(INT_PTR)ID_SETTINGS_CREATOR_LABEL, hInstGlobal, NULL);

    // Fontes só são aplicadas depois que todos os controles foram criados.
    if (fontePreferencias) EnumChildWindows(hwndSettings, AplicarFonteControle, (LPARAM)fontePreferencias);
    AtualizarListaAtalhos(hwndSettings, g_selectedShortcut);
    AtualizarPainelCPU(hwndSettings);
    AplicarCategoriaPreferencias(hwndSettings, 0);
    SetWindowPos(hwndSettings, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow(hwndSettings, SW_SHOW);
    UpdateWindow(hwndSettings);
}

// Funções de Arquivos e Salvamento
static int CALLBACK InicializarDialogoPasta(HWND dialog, UINT msg, LPARAM, LPARAM data) {
    if (msg == BFFM_INITIALIZED && data) {
        const wchar_t* inicial = (const wchar_t*)data;
        if (inicial[0] && GetFileAttributesW(inicial) != INVALID_FILE_ATTRIBUTES)
            SendMessageW(dialog, BFFM_SETSELECTIONW, TRUE, (LPARAM)inicial);
    }
    return 0;
}
bool SelecionarPasta(HWND hwndOwner, wchar_t* outPath) {
    BROWSEINFOW bi{};
    bi.hwndOwner = hwndOwner;
    bi.pszDisplayName = outPath;
    bi.lpszTitle = L"Selecione a pasta do projeto Kodanima";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = InicializarDialogoPasta;
    bi.lParam = (LPARAM)(g_pastaInicialAbrir[0] ? g_pastaInicialAbrir : configAtual.diretorioPasta);
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl != 0) {
        bool ok = SHGetPathFromIDListW(pidl, outPath) != FALSE;
        CoTaskMemFree(pidl);
        return ok;
    }
    return false;
}

HBITMAP CriarBitmapEmBranco(int w, int h, COLORREF corFundo) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hBmpOld = (HBITMAP)SelectObject(hdcMem, hBmp);

    RECT rc = { 0, 0, w, h };
    HBRUSH hBrush = CreateSolidBrush(corFundo);
    FillRect(hdcMem, &rc, hBrush);
    DeleteObject(hBrush);

    SelectObject(hdcMem, hBmpOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hBmp;
}

bool SalvarHBITMAPComoBMP(HBITMAP hbmp, const wchar_t* filename) {
    if (hbmp == NULL || filename == NULL || filename[0] == L'\0') return false;

    BITMAP bmp{};
    if (GetObjectW(hbmp, sizeof(bmp), &bmp) != sizeof(bmp)) return false;
    if (bmp.bmWidth <= 0 || bmp.bmHeight <= 0) return false;

    // BMP de 24 bits: cada linha precisa ter tamanho alinhado a 4 bytes.
    const ULONGLONG rowBytes64 = ((static_cast<ULONGLONG>(bmp.bmWidth) * 3ULL + 3ULL) / 4ULL) * 4ULL;
    const ULONGLONG imageBytes64 = rowBytes64 * static_cast<ULONGLONG>(bmp.bmHeight);
    const ULONGLONG totalBytes64 = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageBytes64;
    if (imageBytes64 == 0 || totalBytes64 > 0xFFFFFFFFULL || imageBytes64 > static_cast<ULONGLONG>(SIZE_MAX)) return false;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = bmp.bmWidth;
    info.bmiHeader.biHeight = bmp.bmHeight; // BMP tradicional: origem inferior
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = static_cast<DWORD>(imageBytes64);
    // BMP armazena densidade em pixels por metro; converte o DPI escolhido pelo usuário.
    const int dpiSeguro = std::max(36, std::min(1200, dpiProjeto));
    info.bmiHeader.biXPelsPerMeter = (LONG)lround(dpiSeguro / 0.0254);
    info.bmiHeader.biYPelsPerMeter = (LONG)lround(dpiSeguro / 0.0254);

    std::vector<BYTE> pixels;
    try {
        pixels.resize(static_cast<size_t>(imageBytes64), 0);
    } catch (...) {
        return false;
    }

    HDC hdc = GetDC(NULL);
    if (hdc == NULL) return false;
    const int linhas = GetDIBits(hdc, hbmp, 0, static_cast<UINT>(bmp.bmHeight),
                                 pixels.data(), &info, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    if (linhas != bmp.bmHeight) return false;

    BITMAPFILEHEADER fileHeader{};
    fileHeader.bfType = 0x4D42; // assinatura "BM"
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = static_cast<DWORD>(totalBytes64);

    HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    bool ok = WriteFile(file, &fileHeader, sizeof(fileHeader), &written, NULL) &&
              written == sizeof(fileHeader);
    if (ok) {
        ok = WriteFile(file, &info.bmiHeader, sizeof(info.bmiHeader), &written, NULL) &&
             written == sizeof(info.bmiHeader);
    }
    if (ok) {
        ok = WriteFile(file, pixels.data(), static_cast<DWORD>(pixels.size()), &written, NULL) &&
             written == pixels.size();
    }
    if (!CloseHandle(file)) ok = false;
    if (!ok) DeleteFileW(filename);
    return ok;
}

HBITMAP CarregarBMPComoHBITMAP(const wchar_t* filename) {
    HBITMAP hBmp = (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hBmp) {
        hBmp = CriarBitmapEmBranco(configAtual.largura, configAtual.altura, configAtual.corFundo);
    }
    return hBmp;
}

// Garante um bitmap independente por camada em cada quadro. hBmpQuadro continua
// sendo o alias da camada ativa para manter compatibilidade com as ferramentas existentes.
void GarantirBitmapsCamadas(FrameData& f) {
    if (timelineLayers.empty()) timelineLayers.push_back({L"Camada 1", true});
    if (f.layerBitmaps.empty()) {
        if (f.hBmpQuadro) f.layerBitmaps.push_back(f.hBmpQuadro);
        else f.layerBitmaps.push_back(CriarBitmapEmBranco(configAtual.largura, configAtual.altura, configAtual.corFundo));
    }
    while (f.layerBitmaps.size() < timelineLayers.size())
        f.layerBitmaps.push_back(CriarBitmapEmBranco(configAtual.largura, configAtual.altura, RGB(255,255,255)));
    while(f.layerDurations.size()<timelineLayers.size()) f.layerDurations.push_back(std::max(0.001,f.segundos));
    if(f.layerFrameExists.empty()) f.layerFrameExists.assign(timelineLayers.size(), true);
    while(f.layerFrameExists.size()<timelineLayers.size()) f.layerFrameExists.push_back(true);
    if(f.layerFrameExists.size()>timelineLayers.size()) f.layerFrameExists.resize(timelineLayers.size());
    if(f.layerDurations.size()>timelineLayers.size()) f.layerDurations.resize(timelineLayers.size());
    while (f.layerBitmaps.size() > timelineLayers.size()) {
        if (f.layerBitmaps.back()) DeleteObject(f.layerBitmaps.back());
        f.layerBitmaps.pop_back();
    }
    activeTimelineLayer = std::max(0, std::min(activeTimelineLayer, (int)f.layerBitmaps.size()-1));
    f.hBmpQuadro = f.layerBitmaps[activeTimelineLayer];
}
void SincronizarAliasCamadaAtiva() {
    if (timelineLayers.empty()) timelineLayers.push_back({L"Camada 1", true});
    activeTimelineLayer = std::max(0, std::min(activeTimelineLayer, (int)timelineLayers.size()-1));
    for (auto& f : listaFrames) GarantirBitmapsCamadas(f);
}
HBITMAP ComporQuadroComCamadas(int idx) {
    if (idx < 0 || idx >= (int)listaFrames.size()) return NULL;
    FrameData& f = listaFrames[idx]; GarantirBitmapsCamadas(f);
    HBITMAP out = CriarBitmapEmBranco(configAtual.largura, configAtual.altura, RGB(255,255,255));
    if (!out) return NULL;
    HDC dst = CreateCompatibleDC(NULL); HBITMAP oldDst=(HBITMAP)SelectObject(dst,out);
    for (int i=(int)timelineLayers.size()-1;i>=0;--i) {
        if (!timelineLayers[i].visible) continue;
        int cel=-1;
        for(int j=std::min(idx,(int)listaFrames.size()-1);j>=0;--j) if(QuadroExisteNaCamada(listaFrames[j],i)){ cel=j; break; }
        if(cel<0) for(int j=idx+1;j<(int)listaFrames.size();++j) if(QuadroExisteNaCamada(listaFrames[j],i)){ cel=j; break; }
        if(cel<0 || i >= (int)listaFrames[cel].layerBitmaps.size() || !listaFrames[cel].layerBitmaps[i]) continue;
        HDC src=CreateCompatibleDC(NULL); HBITMAP oldSrc=(HBITMAP)SelectObject(src,listaFrames[cel].layerBitmaps[i]);
        TransparentBlt(dst,0,0,configAtual.largura,configAtual.altura,src,0,0,configAtual.largura,configAtual.altura,RGB(255,255,255));
        SelectObject(src,oldSrc); DeleteDC(src);
    }
    SelectObject(dst,oldDst); DeleteDC(dst); return out;
}
HBITMAP ComporQuadroNoTempo(double segundos) {
    if(listaFrames.empty()) return NULL;
    HBITMAP out=CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255));
    if(!out) return NULL;
    HDC dst=CreateCompatibleDC(NULL); HBITMAP oldDst=(HBITMAP)SelectObject(dst,out);
    for(int layer=(int)timelineLayers.size()-1;layer>=0;--layer){
        if(!timelineLayers[layer].visible) continue;
        // Cada camada tem sua própria duração total. Quando o tempo solicitado
        // ultrapassa o fim do último quadro dessa camada, ela fica transparente/
        // vazia; não prolongamos automaticamente o último desenho até o fim da régua.
        double acc=0.0; int chosen=-1;
        const double tempoAlvo = std::max(0.0, segundos);
        for(int fi=0;fi<(int)listaFrames.size();++fi){
            GarantirBitmapsCamadas(listaFrames[fi]);
            if(!QuadroExisteNaCamada(listaFrames[fi],layer)) continue;
            double d=DuracaoCamadaQuadro(listaFrames[fi],layer);
            if(tempoAlvo >= acc && tempoAlvo < acc+d){ chosen=fi; break; }
            acc+=d;
        }
        // chosen permanece -1 se a camada já terminou: não desenha nenhum bitmap.
        if(chosen>=0 && chosen<(int)listaFrames.size() && layer<(int)listaFrames[chosen].layerBitmaps.size() && listaFrames[chosen].layerBitmaps[layer]){
            HDC src=CreateCompatibleDC(NULL); HBITMAP oldSrc=(HBITMAP)SelectObject(src,listaFrames[chosen].layerBitmaps[layer]);
            TransparentBlt(dst,0,0,configAtual.largura,configAtual.altura,src,0,0,configAtual.largura,configAtual.altura,RGB(255,255,255));
            SelectObject(src,oldSrc); DeleteDC(src);
        }
    }
    SelectObject(dst,oldDst); DeleteDC(dst); return out;
}
HBITMAP ObterQuadroCompostoCache(int idx) {
    if (idx < 0 || idx >= (int)listaFrames.size()) return NULL;
    if (!g_canvasCompositeCache || g_canvasCompositeCacheDirty || g_canvasCompositeCacheFrame != idx) {
        HBITMAP novo = ComporQuadroComCamadas(idx);
        if (novo) {
            HBITMAP antigo = g_canvasCompositeCache;
            g_canvasCompositeCache = novo;
            g_canvasCompositeCacheFrame = idx;
            g_canvasCompositeCacheDirty = false;
            if (antigo) DeleteObject(antigo);
        }
    }
    // Não reutilizar a imagem de outro quadro se a composição deste falhar.
    if (g_canvasCompositeCacheFrame != idx) return NULL;
    return g_canvasCompositeCache;
}
void ExcluirBitmapsQuadro(FrameData& f) {
    if (!f.layerBitmaps.empty()) {
        for (HBITMAP b : f.layerBitmaps) if (b) DeleteObject(b);
        f.layerBitmaps.clear(); f.hBmpQuadro=NULL;
    } else if (f.hBmpQuadro) { DeleteObject(f.hBmpQuadro); f.hBmpQuadro=NULL; }
}

void SalvarProjetoCompleto() {
    wchar_t caminhoPasta[MAX_PATH] = L"";
    // Ao editar um projeto aberto, salvar sempre na pasta que foi aberta.
    DWORD ativoAttr=caminhoProjetoAtivo[0]?GetFileAttributesW(caminhoProjetoAtivo):INVALID_FILE_ATTRIBUTES;
    if(caminhoProjetoAtivo[0] && ativoAttr!=INVALID_FILE_ATTRIBUTES && (ativoAttr&FILE_ATTRIBUTE_DIRECTORY)) {
        lstrcpynW(caminhoPasta,caminhoProjetoAtivo,MAX_PATH);
    } else {
        wchar_t teste[MAX_PATH];
        wsprintfW(teste, L"%s\\project.json", configAtual.diretorioPasta);
        DWORD attr = GetFileAttributesW(teste);
        wsprintfW(teste, L"%s\\tm", configAtual.diretorioPasta);
        DWORD attrTm = GetFileAttributesW(teste);
        if (attr != INVALID_FILE_ATTRIBUTES || (attrTm != INVALID_FILE_ATTRIBUTES && !(attrTm & FILE_ATTRIBUTE_DIRECTORY))) {
            lstrcpynW(caminhoPasta, configAtual.diretorioPasta, MAX_PATH);
        } else {
            wsprintfW(caminhoPasta, L"%s\\%s", configAtual.diretorioPasta, configAtual.nomeProjeto);
            CreateDirectoryW(caminhoPasta, NULL);
        }
    }

    lstrcpynW(caminhoProjetoAtivo,caminhoPasta,MAX_PATH);
    // Todo projeto novo recebe sua pasta assets, mesmo vazia.
    { std::wstring assetsDir=std::wstring(caminhoPasta)+L"\\assets"; CreateDirectoryW(assetsDir.c_str(),NULL); }
    // Cada camada tem seus próprios BMPs por quadro.
    wchar_t pastaCamadas[MAX_PATH];
    wsprintfW(pastaCamadas, L"%s\\camadas", caminhoPasta); CreateDirectoryW(pastaCamadas, NULL);
    SincronizarAliasCamadaAtiva();
    for (size_t layer=0; layer<timelineLayers.size(); ++layer) {
        wchar_t pastaLayer[MAX_PATH]; wsprintfW(pastaLayer,L"%s\\Camada_%u",pastaCamadas,(unsigned)(layer+1)); CreateDirectoryW(pastaLayer,NULL);
        for (size_t i=0;i<listaFrames.size();++i) {
            GarantirBitmapsCamadas(listaFrames[i]); wchar_t caminhoQuadro[MAX_PATH];
            wsprintfW(caminhoQuadro,L"%s\\quadro_%d_%dms.bmp",pastaLayer,(int)(i+1),(int)(listaFrames[i].segundos*1000.0));
            if (!QuadroExisteNaCamada(listaFrames[i],(int)layer)) continue;
            if (!SalvarHBITMAPComoBMP(listaFrames[i].layerBitmaps[layer],caminhoQuadro)) { MessageBoxW(hwndCanvas,L"Não foi possível salvar um dos quadros da camada.",L"Kodanima",MB_OK|MB_ICONWARNING); return; }
        }
    }

    // Metadados novos em JSON (UTF-8 simples/ASCII para os campos estruturais).
    wchar_t caminhoJson[MAX_PATH];
    wsprintfW(caminhoJson, L"%s\\project.json", caminhoPasta);
    std::wofstream json(caminhoJson, std::ios::out | std::ios::trunc);
    json.imbue(std::locale::classic());
    if (!json.is_open()) {
        MessageBoxW(hwndCanvas, L"Não foi possível gravar project.json.", L"Kodanima", MB_OK | MB_ICONERROR);
        return;
    }
    json << L"{\n  \"format\": \"KodanimaProject\",\n"
         << L"  \"version\": 1,\n"
         << L"  \"width\": " << configAtual.largura << L",\n"
         << L"  \"height\": " << configAtual.altura << L",\n"
         << L"  \"fps\": " << configAtual.fps << L",\n"
         << L"  \"background\": " << (unsigned long)configAtual.corFundo << L",\n"
         << L"  \"selectedFrame\": " << frameAtual << L",\n"
         << L"  \"selectedLayer\": " << selectedTimelineLayer << L",\n"
         << L"  \"playheadSeconds\": " << timelinePlayheadSeconds << L",\n"
         << L"  \"audioPath\": \"" << g_audioRelativePath << L"\",\n"
         << L"  \"audioStartSeconds\": " << g_audioStartSeconds << L",\n"
         << L"  \"audioDurationSeconds\": " << g_audioDurationSeconds << L",\n"
         << L"  \"mediaAssets\": [";
    for(size_t mi=0;mi<g_projectMediaAssets.size();++mi) {
        const KodaProjectMediaAsset& a=g_projectMediaAssets[mi];
        json << L"{\"name\":\"" << a.name << L"\",\"path\":\"" << a.relativePath << L"\",\"type\":\"" << a.type
             << L"\",\"startSeconds\":" << a.startSeconds << L",\"durationSeconds\":" << a.durationSeconds
             << L",\"frame\":" << a.frame << L",\"layer\":" << a.layer
             << L",\"x\":" << a.x << L",\"y\":" << a.y << L"}";
        if(mi+1<g_projectMediaAssets.size()) json << L",";
    }
    json << L"],\n  \"layers\": [";
    for (size_t li=0; li<timelineLayers.size(); ++li) {
        json << L"{\"name\": \"" << timelineLayers[li].name << L"\", \"folder\": \"camadas/Camada_" << (li+1) << L"\", \"visible\": " << (timelineLayers[li].visible?L"true":L"false") << L"}";
        if (li+1<timelineLayers.size()) json << L",";
    }
    json << L"],\n  \"frames\": [\n";
    for (size_t i = 0; i < listaFrames.size(); ++i) {
        GarantirBitmapsCamadas(listaFrames[i]);
        json << L"    {\"id\": " << (i + 1) << L", \"seconds\": " << listaFrames[i].segundos << L", \"durationUnit\": " << listaFrames[i].unidadeDuracao << L", \"durationValue\": " << (listaFrames[i].unidadeDuracao==0 ? listaFrames[i].segundos/60.0 : listaFrames[i].segundos) << L", \"layerExists\": [";
        for(size_t li=0;li<listaFrames[i].layerFrameExists.size();++li){ if(li) json << L", "; json << (listaFrames[i].layerFrameExists[li]?L"true":L"false"); }
        json << L"], \"layerSeconds\": [";
        for(size_t li=0;li<listaFrames[i].layerDurations.size();++li){ if(li) json << L", "; json << listaFrames[i].layerDurations[li]; }
        json << L"]}";
        if (i + 1 < listaFrames.size()) json << L",";
        json << L"\n";
    }
    json << L"  ]\n}\n";
    json.close();
    if (!json) {
        MessageBoxW(hwndCanvas, L"Falha ao finalizar project.json.", L"Kodanima", MB_OK | MB_ICONERROR);
        return;
    }
    // Não cria mais arquivos .tm/.txt nos projetos novos. Os antigos continuam
    // sendo lidos pelo carregador de compatibilidade abaixo.
    DeleteFileW((std::wstring(caminhoPasta) + L"\\tm").c_str());
    DeleteFileW((std::wstring(caminhoPasta) + L"\\tm.txt").c_str());
    // Configurações de ferramentas e timeline pertencem a este projeto.
    SalvarConfiguracoesUsuario();
}

bool CarregarProjetoCompleto(const wchar_t* caminhoPastaRaiz) {
    wchar_t caminhoJson[MAX_PATH];
    wsprintfW(caminhoJson, L"%s\\project.json", caminhoPastaRaiz);
    DWORD jsonAttr = GetFileAttributesW(caminhoJson);
    if (jsonAttr != INVALID_FILE_ATTRIBUTES && !(jsonAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wifstream jf(caminhoJson);
        jf.imbue(std::locale::classic());
        if (!jf.is_open()) return false;
        std::wstring data((std::istreambuf_iterator<wchar_t>(jf)), std::istreambuf_iterator<wchar_t>());
        jf.close();
        lstrcpynW(caminhoProjetoAtivo,caminhoPastaRaiz,MAX_PATH);
        KodaAudioFechar(); g_audioPath.clear(); g_audioRelativePath.clear();
        g_audioStartSeconds=0.0; g_audioDurationSeconds=0.0; g_projectMediaAssets.clear(); g_deletedMediaAssets.clear(); g_selectedMediaAssetIndex=-1;
        auto readString = [&](const std::wstring& source, const wchar_t* key) -> std::wstring {
            std::wstring token=L"\""+std::wstring(key)+L"\"";
            size_t p0=source.find(token); if(p0==std::wstring::npos)return L"";
            p0=source.find(L':',p0+token.size()); if(p0==std::wstring::npos)return L"";
            p0=source.find(L'\"',p0+1); if(p0==std::wstring::npos)return L"";
            size_t q=p0+1; std::wstring out;
            while(q<source.size()) {
                if(source[q]==L'\"') break;
                if(source[q]==L'\\' && q+1<source.size()) { ++q; if(source[q]==L'n')out.push_back(L'\n'); else if(source[q]==L'r')out.push_back(L'\r'); else out.push_back(source[q]); ++q; continue; }
                out.push_back(source[q++]);
            }
            return out;
        };
        auto readDouble = [&](const std::wstring& source,const wchar_t* key,double fallback) -> double {
            std::wstring token=L"\""+std::wstring(key)+L"\""; size_t p0=source.find(token);
            if(p0==std::wstring::npos)return fallback; p0=source.find(L':',p0+token.size());
            if(p0==std::wstring::npos)return fallback; return wcstod(source.c_str()+p0+1,NULL);
        };
        g_audioRelativePath=readString(data,L"audioPath");
        g_audioStartSeconds=std::max(0.0,readDouble(data,L"audioStartSeconds",0.0));
        g_audioDurationSeconds=std::max(0.0,readDouble(data,L"audioDurationSeconds",0.0));
        if(!g_audioRelativePath.empty()) {
            std::wstring rel=g_audioRelativePath; for(size_t c=0;c<rel.size();++c)if(rel[c]==L'/')rel[c]=L'\\';
            g_audioPath=std::wstring(caminhoPastaRaiz)+L"\\"+rel;
            if(GetFileAttributesW(g_audioPath.c_str())==INVALID_FILE_ATTRIBUTES) { g_audioPath.clear(); g_audioRelativePath.clear(); }
        }
        size_t ma=data.find(L"\"mediaAssets\"");
        if(ma!=std::wstring::npos) {
            size_t ar=data.find(L'[',ma), arEnd=(ar==std::wstring::npos)?ar:data.find(L']',ar);
            if(ar!=std::wstring::npos && arEnd!=std::wstring::npos) {
                size_t cur=ar+1;
                while(cur<arEnd) {
                    size_t ob=data.find(L'{',cur); if(ob==std::wstring::npos || ob>=arEnd)break;
                    size_t oe=data.find(L'}',ob); if(oe==std::wstring::npos || oe>=arEnd)break;
                    std::wstring obj=data.substr(ob,oe-ob+1);
                    KodaProjectMediaAsset a; a.name=readString(obj,L"name"); a.relativePath=readString(obj,L"path"); if(a.name.empty()) a.name=KodaNomeBase(a.relativePath.c_str()); a.type=readString(obj,L"type");
                    a.startSeconds=std::max(0.0,readDouble(obj,L"startSeconds",0.0));
                    a.durationSeconds=std::max(0.0,readDouble(obj,L"durationSeconds",0.0));
                    auto readObjInt=[&](const wchar_t* k,int def)->int { return (int)readDouble(obj,k,def); };
                    a.frame=std::max(0,readObjInt(L"frame",0)); a.layer=std::max(0,readObjInt(L"layer",0));
                    a.x=readObjInt(L"x",0); a.y=readObjInt(L"y",0);
                    if(!a.relativePath.empty() && (a.type==L"image"||a.type==L"audio"||a.type==L"video"||a.type==L"gif")) g_projectMediaAssets.push_back(a);
                    cur=oe+1;
                }
            }
        }
        auto readInt = [&](const wchar_t* key, int fallback) -> int {
            std::wstring token = std::wstring(L"\"") + key + L"\"";
            size_t p = data.find(token); if (p == std::wstring::npos) return fallback;
            p = data.find(L':', p + token.size()); if (p == std::wstring::npos) return fallback;
            return _wtoi(data.c_str() + p + 1);
        };
        configAtual.largura = std::max(1, std::min(16384, readInt(L"width", 800)));
        configAtual.altura = std::max(1, std::min(16384, readInt(L"height", 600)));
        configAtual.fps = std::max(1, std::min(240, readInt(L"fps", 24)));
        configAtual.corFundo = (COLORREF)readInt(L"background", (int)RGB(255,255,255));
        timelineLayers.clear();
        size_t lp=data.find(L"\"layers\""); size_t le=(lp==std::wstring::npos)?std::wstring::npos:data.find(L']',lp);
        if(lp!=std::wstring::npos && le!=std::wstring::npos){ size_t np=lp; while((np=data.find(L"\"name\"",np))!=std::wstring::npos && np<le){ size_t q1=data.find(L'\"',data.find(L':',np)+1); size_t q2=(q1==std::wstring::npos)?q1:data.find(L'\"',q1+1); if(q1==std::wstring::npos||q2==std::wstring::npos||q2>le) break; { size_t objEnd=data.find(L'}',q2); std::wstring obj=data.substr(np,objEnd==std::wstring::npos?le-np:objEnd-np); bool vis=(obj.find(L"false")==std::wstring::npos); timelineLayers.push_back({data.substr(q1+1,q2-q1-1),vis}); } np=q2+1; } }
        if(timelineLayers.empty()) timelineLayers.push_back({L"Camada 1",true});
        activeTimelineLayer=selectedTimelineLayer=0;
        InvalidarCacheComposicaoCanvas(); listaFrames.clear();
        size_t pos = data.find(L"\"frames\"");
        if (pos == std::wstring::npos) return false;
        pos = data.find(L'[', pos); if (pos == std::wstring::npos) return false;
        while ((pos = data.find(L"\"id\"", pos)) != std::wstring::npos) {
            size_t idColon = data.find(L':', pos); if (idColon == std::wstring::npos) break;
            int id = _wtoi(data.c_str() + idColon + 1);
            size_t secKey = data.find(L"\"seconds\"", idColon); if (secKey == std::wstring::npos) break;
            size_t secColon = data.find(L':', secKey); if (secColon == std::wstring::npos) break;
            double seconds = wcstod(data.c_str() + secColon + 1, NULL);
            size_t nextFrameKey = data.find(L"\"id\"", secColon + 1);
            size_t unitKey = data.find(L"\"durationUnit\"", secColon + 1);
            int savedUnit = 1;
            if (unitKey != std::wstring::npos && (nextFrameKey == std::wstring::npos || unitKey < nextFrameKey)) {
                size_t uc = data.find(L':', unitKey); if (uc != std::wstring::npos) savedUnit = _wtoi(data.c_str() + uc + 1);
            }
            if (id <= 0) { pos = secColon + 1; continue; }
            FrameData f{}; f.id = id; f.segundos = std::max(0.001, std::min(3600.0, seconds)); f.unidadeDuracao = (savedUnit == 0 ? 0 : 1);
            size_t objEnd=data.find(L'}',secColon); if(objEnd==std::wstring::npos)objEnd=data.size();
            size_t layerSecKey=data.find(L"\"layerSeconds\"",secColon);
            if(layerSecKey!=std::wstring::npos && layerSecKey<objEnd){ size_t arr=data.find(L'[',layerSecKey); size_t arrEnd=(arr==std::wstring::npos)?arr:data.find(L']',arr); if(arr!=std::wstring::npos && arrEnd!=std::wstring::npos && arrEnd<objEnd){ size_t q=arr+1; while(q<arrEnd && f.layerDurations.size()<timelineLayers.size()){ while(q<arrEnd && (data[q]==L' '||data[q]==L'\t'||data[q]==L','))++q; if(q>=arrEnd)break; wchar_t* ep=nullptr; double d=wcstod(data.c_str()+q,&ep); if(ep==data.c_str()+q)break; f.layerDurations.push_back(std::max(0.001,std::min(3600.0,d))); q=(size_t)(ep-data.c_str()); } } }
            while(f.layerDurations.size()<timelineLayers.size()) f.layerDurations.push_back(f.segundos);
            f.layerFrameExists.assign(timelineLayers.size(), true); // projetos antigos: todos os quadros existiam em todas as camadas
            size_t existsKey=data.find(L"\"layerExists\"",secColon);
            if(existsKey!=std::wstring::npos && existsKey<objEnd){ size_t arr=data.find(L'[',existsKey), arrEnd=(arr==std::wstring::npos)?arr:data.find(L']',arr); if(arr!=std::wstring::npos && arrEnd!=std::wstring::npos && arrEnd<objEnd){ size_t q=arr+1; for(size_t li=0;li<timelineLayers.size() && q<arrEnd;++li){ while(q<arrEnd && (data[q]==L' '||data[q]==L'\t'||data[q]==L','))++q; if(q>=arrEnd)break; f.layerFrameExists[li]=(data.compare(q,4,L"true")==0); size_t comma=data.find(L',',q); q=(comma==std::wstring::npos||comma>=arrEnd)?arrEnd:comma+1; } } }
            for(size_t li=0; li<timelineLayers.size(); ++li) {
                wchar_t folder[MAX_PATH], bmpPath[MAX_PATH];
                wsprintfW(folder,L"%s\\camadas\\Camada_%u",caminhoPastaRaiz,(unsigned)(li+1));
                wsprintfW(bmpPath,L"%s\\quadro_%d_%dms.bmp",folder,id,(int)(f.segundos*1000.0));
                HBITMAP layerBmp=NULL;
                if(!f.layerFrameExists[li]) layerBmp=CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255));
                if(f.layerFrameExists[li] && GetFileAttributesW(bmpPath)!=INVALID_FILE_ATTRIBUTES) layerBmp=(HBITMAP)LoadImageW(NULL,bmpPath,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
                if(f.layerFrameExists[li] && !layerBmp && li==0) { wsprintfW(bmpPath,L"%s\\camadas\\Camada_A\\quadro_%d_%dms.bmp",caminhoPastaRaiz,id,(int)(f.segundos*1000.0)); if(GetFileAttributesW(bmpPath)==INVALID_FILE_ATTRIBUTES) wsprintfW(bmpPath,L"%s\\quadro_%d_%dms.bmp",caminhoPastaRaiz,id,(int)(f.segundos*1000.0)); layerBmp=(HBITMAP)LoadImageW(NULL,bmpPath,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION); }
                if(!layerBmp) layerBmp=CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255));
                f.layerBitmaps.push_back(layerBmp);
            }
            f.hBmpQuadro=f.layerBitmaps[std::min(activeTimelineLayer,(int)f.layerBitmaps.size()-1)];
            SalvarEstadoHistorico(f); listaFrames.push_back(f);
            pos = secColon + 1;
        }
        if(!listaFrames.empty()) {
            frameAtual=std::max(0,std::min(readInt(L"selectedFrame",0),(int)listaFrames.size()-1));
            selectedTimelineFrame=frameAtual;
            selectedTimelineLayer=activeTimelineLayer=std::max(0,std::min(readInt(L"selectedLayer",0),(int)timelineLayers.size()-1));
            timelinePlayheadSeconds=std::max(0.0,readDouble(data,L"playheadSeconds",0.0)); previewTimeSeconds=timelinePlayheadSeconds;
            durationUnit=listaFrames[frameAtual].unidadeDuracao;
            SincronizarAliasCamadaAtiva();
        }
        CarregarConfigProjeto(caminhoPastaRaiz);
        if(!g_audioPath.empty()) KodaAudioAbrir(hwndTimeline?hwndTimeline:hwndMain,false);
        return !listaFrames.empty();
    }
    // Compatibilidade: abre projetos antigos que usam o marcador tm.
    wchar_t caminhoTm[MAX_PATH], caminhoTxt[MAX_PATH];
    wsprintfW(caminhoTm, L"%s\\tm", caminhoPastaRaiz);
    wsprintfW(caminhoTxt, L"%s\\tm.txt", caminhoPastaRaiz);

    if(GetFileAttributesW(caminhoTm)==INVALID_FILE_ATTRIBUTES && GetFileAttributesW(caminhoTxt)==INVALID_FILE_ATTRIBUTES) return false;
    KodaAudioFechar(); g_audioPath.clear(); g_audioRelativePath.clear(); g_audioStartSeconds=0.0; g_audioDurationSeconds=0.0; g_projectMediaAssets.clear(); g_deletedMediaAssets.clear(); g_selectedMediaAssetIndex=-1;
    lstrcpynW(caminhoProjetoAtivo,caminhoPastaRaiz,MAX_PATH);

    if (!CopyFileW(caminhoTm, caminhoTxt, FALSE)) {
        // Se tm.txt ja existir de uma tentativa anterior, usa-o; caso contrario falha.
        DWORD at = GetFileAttributesW(caminhoTxt);
        if (at == INVALID_FILE_ATTRIBUTES) return false;
    }
    std::wifstream arquivoTxt(caminhoTxt);
    if (!arquivoTxt.is_open()) return false;

    InvalidarCacheComposicaoCanvas(); listaFrames.clear();
    std::wstring linha;

    while (std::getline(arquivoTxt, linha)) {
        if (linha.rfind(L"W:", 0) == 0) {
            size_t posW = linha.find(L"W:") + 2;
            size_t posH = linha.find(L"|H:");
            size_t posF = linha.find(L"|FPS:");
            if (posW != std::wstring::npos && posH != std::wstring::npos && posF != std::wstring::npos) {
                configAtual.largura = _wtoi(linha.substr(posW, posH - posW).c_str());
                configAtual.altura = _wtoi(linha.substr(posH + 3, posF - (posH + 3)).c_str());
                configAtual.fps = _wtoi(linha.substr(posF + 5).c_str());
            }
        } else if (linha.rfind(L"FRAME:", 0) == 0) {
            size_t posFrame = linha.find(L"FRAME:") + 6;
            size_t posSeg = linha.find(L"|SEGUNDOS:");
            if (posFrame != std::wstring::npos && posSeg != std::wstring::npos) {
                int idFrame = _wtoi(linha.substr(posFrame, posSeg - posFrame).c_str());
                double segs = _wtof(linha.substr(posSeg + 10).c_str());

                FrameData f;
                f.id = idFrame;
                f.segundos = segs;

                wchar_t caminhoImg[MAX_PATH];
                wsprintfW(caminhoImg, L"%s\\quadro_%d_%dms.bmp", caminhoPastaRaiz, idFrame, (int)(segs*1000.0));
                DWORD attrImg = GetFileAttributesW(caminhoImg);
                if (attrImg == INVALID_FILE_ATTRIBUTES) {
                    // Compatibilidade com projetos antigos que gravavam BMP com extensao .png.
                    wsprintfW(caminhoImg, L"%s\\quadro_%d_%ds.png", caminhoPastaRaiz, idFrame, (int)segs);
                }
                f.hBmpQuadro = CarregarBMPComoHBITMAP(caminhoImg);

                // Inicializa histórico com estado inicial
                SalvarEstadoHistorico(f);
                listaFrames.push_back(f);
            }
        }
    }

    arquivoTxt.close();
    DeleteFileW(caminhoTxt);
    return !listaFrames.empty();
}

// ============================================================================
// CRIAÇÃO DAS JANELAS MODULARES (CANVAS, TIMELINE, FERRAMENTAS)
// ============================================================================

void AbrirJanelasModulares() {
    SincronizarAliasCamadaAtiva();
    if (listaFrames.empty()) {
        for (int i = 1; i <= 4; i++) {
            FrameData f;
            f.id = i;
            f.segundos = 1; f.unidadeDuracao=durationUnit;
            f.hBmpQuadro = CriarBitmapEmBranco(configAtual.largura, configAtual.altura, configAtual.corFundo);
            SalvarEstadoHistorico(f);
            listaFrames.push_back(f);
        }
    }

    // 1. Janela de Ferramentas (Esquerda)
    WNDCLASSEXW wcTools{};
    wcTools.cbSize = sizeof(WNDCLASSEXW);
    wcTools.lpfnWndProc = WndProcTools;
    wcTools.hInstance = hInstGlobal;
    wcTools.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wcTools.hIcon = hIconeApp;
    wcTools.hIconSm = hIconeApp;
    wcTools.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcTools.lpszClassName = L"KodanimaToolsClass";
    RegisterClassExW(&wcTools);

    // Ferramentas flutuantes: sem moldura, barra de título ou janela aparente.
    // O retângulo estreito é apenas um fundo discreto atrás dos ícones.
    int alturaPainelFerramentas = 302;
    hwndTools = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"KodanimaToolsClass", L"",
        WS_POPUP | WS_CLIPCHILDREN,
        12, 70, 40, alturaPainelFerramentas, NULL, NULL, hInstGlobal, NULL);

    const int toolX=3, toolW=34, toolH=34, toolGap=5, toolY=5;
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | BS_NOTIFY, toolX, toolY+0*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_LAPIS, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+1*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_BORRACHA, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+2*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_BALDE, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+3*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_LACON, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+4*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_RODA_CORES, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+5*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_TOOL_CONTA_GOTAS, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, toolX, toolY+6*(toolH+toolGap), toolW, toolH, hwndTools, (HMENU)ID_BTN_RENDERIZAR, hInstGlobal, NULL);
    // O botão direito funciona mesmo quando o cursor está sobre um botão filho.
    for (int id : {ID_TOOL_LAPIS, ID_TOOL_BORRACHA, ID_TOOL_BALDE, ID_TOOL_LACON}) {
        HWND hb = GetDlgItem(hwndTools, id);
        if (hb) SetWindowSubclass(hb, SubclassBotaoFerramenta, 1, (DWORD_PTR)id);
    }
    KodaCriarDicasFerramentas(hwndTools);

    // 2. Janela Principal do Canvas (Centro)
    WNDCLASSEXW wcCanvas{};
    wcCanvas.cbSize = sizeof(WNDCLASSEXW);
    wcCanvas.lpfnWndProc = WndProcCanvas;
    wcCanvas.hInstance = hInstGlobal;
    wcCanvas.hCursor = cursorPersonalizado ? cursorPersonalizado : LoadCursorW(NULL, MAKEINTRESOURCEW(32515));
    wcCanvas.hIcon = hIconeApp;
    wcCanvas.hIconSm = hIconeApp;
    wcCanvas.hbrBackground = (HBRUSH)CreateSolidBrush(CorTemaFundo());
    wcCanvas.lpszClassName = L"KodanimaCanvasClass";
    RegisterClassExW(&wcCanvas);

    hwndCanvas = CreateWindowW(L"KodanimaCanvasClass", L"Kodanima 2D - Canvas Principal (Ctrl+S Salva)",
        // Barra de título com apenas Minimizar e Fechar; sem botão Maximizar.
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstGlobal, NULL);

    // 3. Janela da Linha do Tempo (Inferior)
    WNDCLASSEXW wcTime{};
    wcTime.cbSize = sizeof(WNDCLASSEXW);
    wcTime.style = CS_DBLCLKS;
    wcTime.lpfnWndProc = WndProcTimeline;
    wcTime.hInstance = hInstGlobal;
    wcTime.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wcTime.hIcon = hIconeApp;
    wcTime.hIconSm = hIconeApp;
    wcTime.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcTime.lpszClassName = L"KodanimaTimelineClass";
    RegisterClassExW(&wcTime);

    RECT workAreaTimeline{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&workAreaTimeline,0);
    int larguraTelaTimeline = workAreaTimeline.right-workAreaTimeline.left;
    int alturaTelaTimeline = workAreaTimeline.bottom-workAreaTimeline.top;
    int larguraTimeline = std::min(900, std::max(650, larguraTelaTimeline - 110));
    timelineDefaultHeight=285;
    hwndTimeline = CreateWindowW(L"KodanimaTimelineClass", L"Kodanima | Timeline",
        WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        std::max(workAreaTimeline.left,workAreaTimeline.left+(larguraTelaTimeline-larguraTimeline)/2), std::max(workAreaTimeline.top, workAreaTimeline.bottom - timelineDefaultHeight), larguraTimeline, timelineDefaultHeight, NULL, NULL, hInstGlobal, NULL);

    if (hwndMain && IsWindow(hwndMain)) { DragAcceptFiles(hwndMain,TRUE); SetWindowSubclass(hwndMain,KodaDropSubclassProc,0x4B44,0); }
    if (hwndCanvas) { DragAcceptFiles(hwndCanvas,TRUE); SetWindowSubclass(hwndCanvas,KodaDropSubclassProc,0x4B44,0); EnumChildWindows(hwndCanvas, InstalarSubclassFocoHover, 0); EnumChildWindows(hwndCanvas,KodaAtivarDropNosFilhos,0); }
    if (hwndTools) { DragAcceptFiles(hwndTools,TRUE); SetWindowSubclass(hwndTools,KodaDropSubclassProc,0x4B44,0); EnumChildWindows(hwndTools, InstalarSubclassFocoHover, 0); EnumChildWindows(hwndTools,KodaAtivarDropNosFilhos,0); }
    if (hwndTimeline) { DragAcceptFiles(hwndTimeline,TRUE); SetWindowSubclass(hwndTimeline,KodaDropSubclassProc,0x4B44,0); EnumChildWindows(hwndTimeline, InstalarSubclassFocoHover, 0); EnumChildWindows(hwndTimeline,KodaAtivarDropNosFilhos,0); }
    if (fontePreferencias) {
        if (hwndCanvas) EnumChildWindows(hwndCanvas, AplicarFonteControle, (LPARAM)fontePreferencias);
        if (hwndTools) EnumChildWindows(hwndTools, AplicarFonteControle, (LPARAM)fontePreferencias);
        if (hwndTimeline) EnumChildWindows(hwndTimeline, AplicarFonteControle, (LPARAM)fontePreferencias);
    }
    if (hwndCanvas) SetClassLongPtrW(hwndCanvas,GCLP_HBRBACKGROUND,(LONG_PTR)CreateSolidBrush(CorTemaFundo()));
    AplicarEscalaFerramentas();
    ShowWindow(hwndTools, SW_SHOW);
    if(hwndCanvas) {
        // Detecta uma vez a barra do Windows em ocultação automática e deixa 2 px
        // livres na borda esquerda/direita antes de maximizar o canvas.
        KodaVerificarBordaBarraTarefasUmaVez();
        SetWindowPos(hwndCanvas,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        ShowWindow(hwndCanvas, SW_SHOWMAXIMIZED);
        UpdateWindow(hwndCanvas);
    }
    AplicarCursor(hwndTools);
    AplicarCursor(hwndCanvas);
    AplicarCursor(hwndTimeline);
    ShowWindow(hwndTimeline, SW_SHOW);
    // Oculta a tela inicial durante a edição para não ficar uma janela extra aberta.
    // Fechar o canvas chama FecharTodasJanelas(), que restaura a tela inicial.
    if (hwndMain && IsWindow(hwndMain)) ShowWindow(hwndMain, SW_HIDE);
    // Painéis permanecem acima do canvas para que não sejam encobertos ao desenhar.
    SetWindowPos(hwndTools, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(hwndTimeline, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    if (hwndTimeline && !listaFrames.empty())
        SetWindowTextW(GetDlgItem(hwndTimeline,ID_EDIT_FRAME_SECONDS), std::to_wstring(listaFrames[frameAtual].segundos).c_str());
}

// ============================================================================
// PROCEDIMENTOS DE JANELA (CALLBACKS)
// ============================================================================

static void ReativarTimerFocoAutomatico() {
    if (hwndMain && IsWindow(hwndMain)) {
        SetTimer(hwndMain, ID_TIMER_AUTO_HOVER_FOCUS, 60, NULL);
        g_timerFocoAtivo = true;
        g_ticksMouseParado = 0;
    }
}
static LRESULT CALLBACK SubclassReativarFocoMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    if (msg == WM_MOUSEMOVE) ReativarTimerFocoAutomatico();
    return DefSubclassProc(hwnd, msg, wp, lp);
}
static BOOL CALLBACK InstalarSubclassFocoHover(HWND child, LPARAM) {
    SetWindowSubclass(child, SubclassReativarFocoMouse, 0xA911, 0);
    return TRUE;
}

// Seleciona automaticamente a janela lógica sob o cursor, sem exigir clique.
// A troca só ocorre quando se entra em outra janela, evitando SetForegroundWindow
// repetido e reduzindo custo durante o movimento da caneta.
static void AtualizarFocoAutomaticoPorHover() {
    POINT pt{};
    if (!GetCursorPos(&pt)) return;
    HWND under=WindowFromPoint(pt);
    HWND target=NULL;
    HWND candidates[]={hwndCanvas, hwndTimeline, hwndTools, hwndShortcutPanel, hwndMain};
    for (HWND root : candidates) {
        if (root && IsWindow(root) && IsWindowVisible(root) &&
            (under==root || IsChild(root,under))) { target=root; break; }
    }
    if (!target) { g_hoverFocusedRoot=NULL; return; }
    if (target==g_hoverFocusedRoot) return;
    g_hoverFocusedRoot=target;
    // Não rouba foco de um campo de texto quando o cursor já está nele; a
    // janela lógica é ativada e o foco do controle permanece para edição normal.
    HWND focused=GetFocus();
    wchar_t cls[64]={0};
    if (focused && IsWindow(focused)) GetClassNameW(focused,cls,63);
    const bool focusInsideTarget = focused && (focused == target || IsChild(target, focused));
    bool editing = focusInsideTarget &&
        (target == hwndShortcutPanel || lstrcmpiW(cls, L"Edit") == 0 ||
         lstrcmpiW(cls, L"ComboBox") == 0 || wcsstr(cls, L"RichEdit") != NULL);
    // Não solicita a janela em primeiro plano enquanto outro programa está ativo:
    // SetForegroundWindow repetido/negado pelo Windows causa o ícone piscando na barra.
    // A janela recebe foco lógico quando o próprio aplicativo já está em primeiro plano.
    HWND foreground = GetForegroundWindow();
    bool appForeground = foreground && (foreground==hwndMain || foreground==hwndCanvas ||
        foreground==hwndTimeline || foreground==hwndTools || foreground==hwndShortcutPanel ||
        IsChild(hwndMain,foreground) || IsChild(hwndCanvas,foreground) ||
        IsChild(hwndTimeline,foreground) || IsChild(hwndTools,foreground) ||
        IsChild(hwndShortcutPanel,foreground));
    if (appForeground) {
        SetActiveWindow(target);
        if (!editing) SetFocus(target);
    }
}

LRESULT CALLBACK WndProcMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_MOUSEMOVE) ReativarTimerFocoAutomatico();
    switch (msg) {
        case WM_CREATE:
            hwndMain = hwnd;
            SetTimer(hwnd, ID_TIMER_AUTO_HOVER_FOCUS, 60, NULL);
            g_timerFocoAtivo = true; g_ticksMouseParado = 0; GetCursorPos(&g_ultimaPosicaoMouseFoco);
            AplicarCursor(hwnd);
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(30, 30, 32)));
            CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 178, 112, 76, 76, hwnd, (HMENU)ID_BTN_CRIAR, hInstGlobal, NULL);
            CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 266, 112, 76, 76, hwnd, (HMENU)ID_BTN_ABRIR, hInstGlobal, NULL);
            CreateWindowW(L"BUTTON", L"Preferências", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 178, 226, 164, 40, hwnd, (HMENU)ID_BTN_SETTINGS, hInstGlobal, NULL);
            CreateWindowW(L"BUTTON", L"Language", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 178, 278, 164, 36, hwnd, (HMENU)ID_BTN_LANGUAGE, hInstGlobal, NULL);
            GetWindowRect(GetDlgItem(hwnd,ID_BTN_CRIAR),&g_startupCreateBaseRect); MapWindowPoints(NULL,hwnd,(POINT*)&g_startupCreateBaseRect,2);
            GetWindowRect(GetDlgItem(hwnd,ID_BTN_ABRIR),&g_startupOpenBaseRect); MapWindowPoints(NULL,hwnd,(POINT*)&g_startupOpenBaseRect,2);
            EnumChildWindows(hwnd, InstalarSubclassFocoHover, 0);
            break;
        case WM_CTLCOLORSTATIC: {
            HDC dc=(HDC)wParam; HWND control=(HWND)lParam;
            break;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis=(DRAWITEMSTRUCT*)lParam;
            if(dis && dis->CtlID==ID_BTN_LANGUAGE){
                HDC dc=dis->hDC; RECT r=dis->rcItem;
                COLORREF bg=(dis->itemState&ODS_SELECTED)?RGB(48,53,65):RGB(38,42,51);
                HBRUSH b=CreateSolidBrush(bg); FillRect(dc,&r,b); DeleteObject(b);
                HPEN p=CreatePen(PS_SOLID,1,RGB(77,85,101)); HGDIOBJ oldp=SelectObject(dc,p); HGDIOBJ oldb=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); Rectangle(dc,r.left,r.top,r.right,r.bottom); SelectObject(dc,oldp); SelectObject(dc,oldb); DeleteObject(p);
                std::wstring label=KodaTraduzirTexto(L"Language"); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(225,230,240));
                RECT tr=r; tr.left+=4; DrawTextW(dc,label.c_str(),-1,&tr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                if(g_startupAnimType==3){
                    const int frame=std::max(0,g_startupAnimFrame); const int radius=10;
                    int px=r.left+13+std::min(11,frame)*12; int py=(r.top+r.bottom)/2;
                    // Pequenos pontos e o personagem amarelo fazem uma animação
                    // curta de Pac-Man a "comer" o rótulo Language.
                    for(int d=0;d<4;++d){ int dx=px-20+d*9; if(dx>r.left+4 && dx<px-5){ HBRUSH dot=CreateSolidBrush(RGB(245,210,105)); RECT dr={dx,py-2,dx+4,py+2}; FillRect(dc,&dr,dot); DeleteObject(dot); } }
                    HBRUSH yellow=CreateSolidBrush(RGB(255,213,45)); HGDIOBJ oldBrush=SelectObject(dc,yellow); Ellipse(dc,px-radius,py-radius,px+radius,py+radius); SelectObject(dc,oldBrush); DeleteObject(yellow);
                    POINT mouth[3]={{px,py},{px+radius+1,py-6},{px+radius+1,py+6}}; HBRUSH cut=CreateSolidBrush(bg); HGDIOBJ ob=SelectObject(dc,cut); HPEN np=CreatePen(PS_SOLID,1,bg); HGDIOBJ op=SelectObject(dc,np); Polygon(dc,mouth,3); SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(np); DeleteObject(cut);
                    HBRUSH eye=CreateSolidBrush(RGB(32,35,42)); HGDIOBJ oe=SelectObject(dc,eye); Ellipse(dc,px+1,py-7,px+5,py-3); SelectObject(dc,oe); DeleteObject(eye);
                }
                if(dis->itemState&ODS_FOCUS) DrawFocusRect(dc,&r); return TRUE;
            }
            if(dis && (dis->CtlID==ID_BTN_CRIAR || dis->CtlID==ID_BTN_ABRIR)){
                HDC dc=dis->hDC; RECT r=dis->rcItem;
                COLORREF bg=(dis->itemState & ODS_SELECTED)?RGB(48,53,65):RGB(38,42,51);
                HBRUSH br=CreateSolidBrush(bg); FillRect(dc,&r,br); DeleteObject(br);
                HPEN pen=CreatePen(PS_SOLID,1,RGB(77,85,101)); HGDIOBJ op=SelectObject(dc,pen); HGDIOBJ ob=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
                Rectangle(dc,r.left,r.top,r.right,r.bottom); SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(pen);
                int cx=(r.left+r.right)/2, cy=(r.top+r.bottom)/2;
                HPEN ip=CreatePen(PS_SOLID,3,RGB(225,230,240)); HGDIOBJ old=SelectObject(dc,ip);
                if(dis->CtlID==ID_BTN_CRIAR){
                    double a=(g_startupAnimType==1 ? g_startupAnimFrame*0.78539816339 : 0.0);
                    int arm=(g_startupAnimType==1 ? 13+g_startupAnimFrame/2 : 14);
                    int dx=(int)(cos(a)*arm), dy=(int)(sin(a)*arm);
                    MoveToEx(dc,cx-dx,cy-dy,NULL); LineTo(dc,cx+dx,cy+dy);
                    MoveToEx(dc,cx+dy,cy-dx,NULL); LineTo(dc,cx-dy,cy+dx);
                } else {
                    int bounce=(g_startupAnimType==2 ? (g_startupAnimFrame%2 ? -5 : 3) : 0);
                    HGDIOBJ oldb=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); Ellipse(dc,cx-19,cy-12+bounce,cx+19,cy+12+bounce);
                    HBRUSH pupil=CreateSolidBrush(RGB(225,230,240)); SelectObject(dc,pupil); Ellipse(dc,cx-4,cy-4+bounce,cx+4,cy+4+bounce); SelectObject(dc,oldb); DeleteObject(pupil);
                }
                SelectObject(dc,old); DeleteObject(ip); if(dis->itemState & ODS_FOCUS) DrawFocusRect(dc,&r); return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps); RECT rc{}; GetClientRect(hwnd,&rc);
            HBRUSH bg=CreateSolidBrush(RGB(30,30,32)); FillRect(dc,&rc,bg); DeleteObject(bg);
            // Logotipo do Windows bem pequeno, discreto, no canto inferior direito.
            int x=rc.right-23, y=rc.bottom-17; const COLORREF colors[4]={RGB(244,83,78),RGB(112,190,96),RGB(74,154,226),RGB(242,190,66)};
            for(int i=0;i<4;++i){ int xx=x+(i%2)*8, yy=y+(i/2)*7; HBRUSH b=CreateSolidBrush(colors[i]); RECT q{xx,yy,xx+7,yy+6}; FillRect(dc,&q,b); DeleteObject(b); }
            EndPaint(hwnd,&ps); return 0;
        }
        case WM_TIMER:
            if (wParam == ID_TIMER_AUTO_HOVER_FOCUS) {
                POINT now{};
                if (GetCursorPos(&now)) {
                    if (now.x == g_ultimaPosicaoMouseFoco.x && now.y == g_ultimaPosicaoMouseFoco.y) {
                        if (++g_ticksMouseParado >= 12) {
                            KillTimer(hwnd, ID_TIMER_AUTO_HOVER_FOCUS);
                            g_timerFocoAtivo = false;
                            return 0;
                        }
                    } else { g_ultimaPosicaoMouseFoco = now; g_ticksMouseParado = 0; }
                }
                AtualizarFocoAutomaticoPorHover();
                return 0;
            }
            if (wParam == ID_TIMER_STARTUP_ANIMATION) {
                ++g_startupAnimFrame;
                if(g_startupAnimType==3){
                    HWND languageButton=GetDlgItem(hwnd,ID_BTN_LANGUAGE);
                    if(languageButton) InvalidateRect(languageButton,NULL,TRUE);
                    if(g_startupAnimFrame>=12){
                        KillTimer(hwnd,ID_TIMER_STARTUP_ANIMATION);
                        g_startupAnimType=0; g_startupAnimFrame=0;
                        AbrirSeletorIdioma(hwnd);
                    }
                    return 0;
                }
                int id=(g_startupAnimType==1?ID_BTN_CRIAR:ID_BTN_ABRIR);
                HWND b=GetDlgItem(hwnd,id);
                if (b && IsWindow(b)) {
                    RECT base=(g_startupAnimType==1?g_startupCreateBaseRect:g_startupOpenBaseRect);
                    int cx=(base.left+base.right)/2, cy=(base.top+base.bottom)/2;
                    int factor=(g_startupAnimType==1 ? (g_startupAnimFrame<=4?100+g_startupAnimFrame*9:136-(g_startupAnimFrame-4)*9) : 100);
                    int w=(base.right-base.left)*factor/100, h=(base.bottom-base.top)*factor/100;
                    int y=base.top;
                    if(g_startupAnimType==2) y += (g_startupAnimFrame<=4 ? -g_startupAnimFrame*3 : -(8-g_startupAnimFrame)*3);
                    SetWindowPos(b,NULL,cx-w/2,y+(base.bottom-base.top-h)/2,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
                    InvalidateRect(b,NULL,TRUE);
                }
                if(g_startupAnimFrame>=8) {
                    KillTimer(hwnd,ID_TIMER_STARTUP_ANIMATION);
                    if(b) { RECT base=(g_startupAnimType==1?g_startupCreateBaseRect:g_startupOpenBaseRect); SetWindowPos(b,NULL,base.left,base.top,base.right-base.left,base.bottom-base.top,SWP_NOZORDER|SWP_NOACTIVATE); InvalidateRect(b,NULL,TRUE); }
                    int action=(g_startupAnimType==1?ID_BTN_CRIAR:ID_BTN_ABRIR); g_startupAnimType=0; g_startupAnimFrame=0;
                    g_startupAnimationDispatch=true; SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(action,BN_CLICKED),(LPARAM)b); g_startupAnimationDispatch=false;
                }
                return 0;
            }
            break;
        case WM_COMMAND:
            if ((LOWORD(wParam) == ID_BTN_CRIAR || LOWORD(wParam) == ID_BTN_ABRIR) && !g_startupAnimationDispatch) {
                if(g_startupAnimType) return 0;
                g_startupAnimType=(LOWORD(wParam)==ID_BTN_CRIAR?1:2); g_startupAnimFrame=0;
                SetTimer(hwnd,ID_TIMER_STARTUP_ANIMATION,25,NULL); InvalidateRect(GetDlgItem(hwnd,LOWORD(wParam)),NULL,TRUE); return 0;
            }
            if(LOWORD(wParam)==ID_BTN_LANGUAGE){
                if(g_startupAnimType) return 0;
                g_startupAnimType=3; g_startupAnimFrame=0;
                SetTimer(hwnd,ID_TIMER_STARTUP_ANIMATION,25,NULL);
                InvalidateRect(GetDlgItem(hwnd,ID_BTN_LANGUAGE),NULL,TRUE); return 0;
            }
            if (LOWORD(wParam) == ID_BTN_CRIAR) {
                // Janela de configuração
                WNDCLASSEXW wc{};
                wc.cbSize = sizeof(WNDCLASSEXW);
                wc.lpfnWndProc = WndProcSetup;
                wc.hInstance = hInstGlobal;
                wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
                wc.hIcon = hIconeApp;
                wc.hIconSm = hIconeApp;
                wc.hbrBackground = CreateSolidBrush(RGB(30,30,32));
                wc.lpszClassName = L"KodanimaSetupClass";
                RegisterClassExW(&wc);

                HWND hwndSetup = CreateWindowW(L"KodanimaSetupClass", L"Novo projeto — configuração da tela", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 620, 590, hwnd, NULL, hInstGlobal, NULL);
                CreateWindowW(L"STATIC", L"Largura (X):", WS_VISIBLE | WS_CHILD, 34, 28, 125, 22, hwndSetup, NULL, hInstGlobal, NULL);
                CreateWindowW(L"EDIT", std::to_wstring(ultimoProjetoW).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 170, 26, 400, 26, hwndSetup, (HMENU)ID_EDIT_LARGURA, hInstGlobal, NULL);
                CreateWindowW(L"STATIC", L"Altura (Y):", WS_VISIBLE | WS_CHILD, 34, 72, 125, 22, hwndSetup, NULL, hInstGlobal, NULL);
                CreateWindowW(L"EDIT", std::to_wstring(ultimoProjetoH).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 170, 70, 400, 26, hwndSetup, (HMENU)ID_EDIT_ALTURA, hInstGlobal, NULL);
                CreateWindowW(L"STATIC", L"FPS (Taxa):", WS_VISIBLE | WS_CHILD, 34, 116, 125, 22, hwndSetup, NULL, hInstGlobal, NULL);
                CreateWindowW(L"EDIT", std::to_wstring(ultimoProjetoFPS).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 170, 114, 400, 26, hwndSetup, (HMENU)ID_EDIT_FPS, hInstGlobal, NULL);
                CreateWindowW(L"STATIC", L"Nome Pasta:", WS_VISIBLE | WS_CHILD, 34, 160, 125, 22, hwndSetup, NULL, hInstGlobal, NULL);
                CreateWindowW(L"EDIT", L"MeuProjetoAnimacao", WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 158, 400, 26, hwndSetup, (HMENU)ID_EDIT_NOME, hInstGlobal, NULL);
                CreateWindowW(L"STATIC", L"Diretório Raiz:", WS_VISIBLE | WS_CHILD, 34, 204, 125, 22, hwndSetup, NULL, hInstGlobal, NULL);
                hwndEditDir = CreateWindowW(L"EDIT", configAtual.diretorioPasta, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 170, 202, 320, 26, hwndSetup, (HMENU)ID_EDIT_DIR, hInstGlobal, NULL);
                CreateWindowW(L"BUTTON", L"Buscar...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 500, 202, 70, 26, hwndSetup, (HMENU)ID_BTN_SELECIONAR_DIR, hInstGlobal, NULL);
                CreateWindowW(L"STATIC",L"Predefinição:",WS_VISIBLE|WS_CHILD,34,246,125,22,hwndSetup,NULL,hInstGlobal,NULL);
                HWND preset=CreateWindowW(L"COMBOBOX",L"",WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL,170,242,400,170,hwndSetup,(HMENU)ID_SETUP_PRESET,hInstGlobal,NULL);
                const wchar_t* presets[]={L"Personalizada",L"HD horizontal — 1280 × 720",L"Full HD horizontal — 1920 × 1080",L"4K horizontal — 3840 × 2160",L"Cinema 4K — 4096 × 2160",L"Instagram vertical HD — 720 × 1280",L"Instagram vertical Full HD — 1080 × 1920",L"Instagram quadrado — 1080 × 1080"};
                for(int pi=0;pi<8;++pi) SendMessageW(preset,CB_ADDSTRING,0,(LPARAM)presets[pi]); SendMessageW(preset,CB_SETCURSEL,0,0);
                CreateWindowW(L"STATIC",L"DPI:",WS_VISIBLE|WS_CHILD,34, 290, 125, 22,hwndSetup,NULL,hInstGlobal,NULL);
                CreateWindowW(L"EDIT",std::to_wstring(dpiProjeto).c_str(),WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,170, 286, 120, 26,hwndSetup,(HMENU)ID_SETUP_DPI,hInstGlobal,NULL);
                CreateWindowW(L"STATIC",L"Pasta completa do projeto:",WS_VISIBLE|WS_CHILD,34, 334, 540, 24,hwndSetup,NULL,hInstGlobal,NULL);
                CreateWindowW(L"STATIC",L"",WS_VISIBLE|WS_CHILD|SS_LEFT,34, 362, 540, 54,hwndSetup,(HMENU)ID_SETUP_FULLPATH,hInstGlobal,NULL);
                { wchar_t pathPreview[MAX_PATH*2]; wchar_t namePreview[256]; GetDlgItemTextW(hwndSetup,ID_EDIT_NOME,namePreview,256); wsprintfW(pathPreview,L"%s\\%s",configAtual.diretorioPasta,namePreview); SetWindowTextW(GetDlgItem(hwndSetup,ID_SETUP_FULLPATH),pathPreview); }
                CreateWindowW(L"BUTTON", L"PRONTO", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 225, 448, 180, 38, hwndSetup, (HMENU)IDOK, hInstGlobal, NULL);
                ShowWindow(hwndSetup, SW_SHOW);
                UpdateWindow(hwndSetup);
            } else if (LOWORD(wParam) == ID_BTN_SETTINGS) { AbrirConfiguracoesUsuario(hwnd);
            } else if (LOWORD(wParam) == ID_BTN_ABRIR) {
                wchar_t pasta[MAX_PATH] = L"";
                lstrcpynW(g_pastaInicialAbrir, caminhoProjetoAtivo[0] ? caminhoProjetoAtivo : configAtual.diretorioPasta, MAX_PATH);
                if (SelecionarPasta(hwnd, pasta)) {
                    if (CarregarProjetoCompleto(pasta)) {
                        // Mantém PastaProjetos como diretório-base para novos projetos.
                        // Nunca gravar a pasta do projeto aberto como raiz de criação,
                        // pois isso provoca estruturas do tipo Projeto\Projeto\... .
                        AbrirJanelasModulares();
                    } else {
                        MessageBoxW(hwnd, L"Não foi possível abrir o projeto. Verifique se a pasta contém o arquivo 'tm'.",
                                    L"Kodanima", MB_OK | MB_ICONWARNING);
                    }
                }
            }
            break;
        case WM_CLOSE:
            if (hwndCanvas || hwndTimeline || hwndTools) {
                HWND c=hwndCanvas, t=hwndTimeline, tools=hwndTools;
                hwndCanvas=hwndTimeline=hwndTools=NULL;
                DestruirBackbufferCanvas();
                if (previewAtivo && c) KillTimer(c, ID_TIMER_PREVIEW);
                previewAtivo=false;
                if(c && IsWindow(c)) DestroyWindow(c);
                if(t && IsWindow(t)) DestroyWindow(t);
                if(tools && IsWindow(tools)) DestroyWindow(tools);
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_AUTO_HOVER_FOCUS);
            DestruirDirectX9(); DestruirDirectX11(); DestruirDirectX12();
            if(g_modD3D11){FreeLibrary(g_modD3D11);g_modD3D11=NULL;} if(g_modD3D12){FreeLibrary(g_modD3D12);g_modD3D12=NULL;} if(g_modDXGI){FreeLibrary(g_modDXGI);g_modDXGI=NULL;}
            if (hJobLimiteMemoria) { CloseHandle(hJobLimiteMemoria); hJobLimiteMemoria=NULL; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH setupDarkBrush = NULL;
    static HBRUSH setupEditBrush = NULL;
    if(!setupDarkBrush) setupDarkBrush=CreateSolidBrush(RGB(30,30,32));
    if(!setupEditBrush) setupEditBrush=CreateSolidBrush(RGB(43,45,52));
    if(msg==WM_CTLCOLORSTATIC) {
        HDC dc=(HDC)wParam; HWND control=(HWND)lParam;
        if(GetDlgCtrlID(control)==ID_SETUP_FULLPATH) { SetTextColor(dc,RGB(90,225,125)); SetBkColor(dc,RGB(30,30,32)); return (LRESULT)setupDarkBrush; }
        SetTextColor(dc,RGB(225,228,236)); SetBkMode(dc,TRANSPARENT); return (LRESULT)setupDarkBrush;
    }
    if(msg==WM_CTLCOLOREDIT || msg==WM_CTLCOLORLISTBOX) {
        HDC dc=(HDC)wParam; SetTextColor(dc,RGB(225,228,236)); SetBkColor(dc,RGB(43,45,52)); return (LRESULT)setupEditBrush;
    }
    switch (msg) {
        case WM_COMMAND:
            if((LOWORD(wParam)==ID_EDIT_NOME || LOWORD(wParam)==ID_EDIT_DIR) && HIWORD(wParam)==EN_CHANGE){
                wchar_t root[MAX_PATH]=L"",name[256]=L"",pathPreview[MAX_PATH*2]=L""; GetDlgItemTextW(hwnd,ID_EDIT_DIR,root,MAX_PATH);GetDlgItemTextW(hwnd,ID_EDIT_NOME,name,256);wsprintfW(pathPreview,L"%s\\%s",root,name);SetWindowTextW(GetDlgItem(hwnd,ID_SETUP_FULLPATH),pathPreview);return 0;
            }
            if(LOWORD(wParam)==ID_SETUP_PRESET && HIWORD(wParam)==CBN_SELCHANGE){
                int idx=(int)SendMessageW(GetDlgItem(hwnd,ID_SETUP_PRESET),CB_GETCURSEL,0,0); int w=ultimoProjetoW,h=ultimoProjetoH;
                switch(idx){case 1:w=1280;h=720;break;case 2:w=1920;h=1080;break;case 3:w=3840;h=2160;break;case 4:w=4096;h=2160;break;case 5:w=720;h=1280;break;case 6:w=1080;h=1920;break;case 7:w=1080;h=1080;break;default:break;}
                if(idx>0){SetDlgItemInt(hwnd,ID_EDIT_LARGURA,w,FALSE);SetDlgItemInt(hwnd,ID_EDIT_ALTURA,h,FALSE);} return 0;
            }
            if (LOWORD(wParam) == ID_BTN_SELECIONAR_DIR) {
                wchar_t pasta[MAX_PATH] = L"";
                lstrcpynW(g_pastaInicialAbrir, configAtual.diretorioPasta, MAX_PATH);
                if (SelecionarPasta(hwnd, pasta)) {
                    SetWindowTextW(hwndEditDir, pasta);
                }
                return 0;
            }
            if (LOWORD(wParam) == IDOK) {
                configAtual.largura = GetDlgItemInt(hwnd, ID_EDIT_LARGURA, NULL, FALSE);
                configAtual.altura = GetDlgItemInt(hwnd, ID_EDIT_ALTURA, NULL, FALSE);
                configAtual.fps = GetDlgItemInt(hwnd, ID_EDIT_FPS, NULL, FALSE);
                dpiProjeto=GetDlgItemInt(hwnd,ID_SETUP_DPI,NULL,FALSE); if(dpiProjeto<36||dpiProjeto>1200)dpiProjeto=96;
                ultimoProjetoW=configAtual.largura; ultimoProjetoH=configAtual.altura; ultimoProjetoFPS=configAtual.fps;
                GetDlgItemTextW(hwnd, ID_EDIT_NOME, configAtual.nomeProjeto, 256);
                GetDlgItemTextW(hwnd, ID_EDIT_DIR, configAtual.diretorioPasta, MAX_PATH);
                SalvarConfiguracoesUsuario();

                if (configAtual.largura < 1 || configAtual.largura > 10000 ||
                    configAtual.altura < 1 || configAtual.altura > 10000 ||
                    configAtual.fps < 1 || configAtual.fps > 120 ||
                    configAtual.nomeProjeto[0] == L'\0' ||
                    configAtual.diretorioPasta[0] == L'\0') {
                    MessageBoxW(hwnd, L"Confira largura, altura, FPS, nome e diretório.",
                                L"Dados inválidos", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                SalvarConfiguracoesUsuario();
                // Cada projeto começa sem mídias da sessão anterior. Os arquivos físicos
                // permanecem na pasta assets do projeto antigo, mas não são herdados.
                KodaAudioFechar(); g_audioPath.clear(); g_audioRelativePath.clear();
                g_audioStartSeconds=0.0; g_audioDurationSeconds=0.0; g_projectMediaAssets.clear(); g_deletedMediaAssets.clear(); g_selectedMediaAssetIndex=-1;
                g_audioClipDragging=false; g_visualClipDragging=false; g_visualClipDragIndex=-1;
                timelinePlayheadSeconds=0.0; previewTimeSeconds=0.0; timelineScrollSeconds=0.0; timelineLayerScroll=0;
                if(bitmapClipboardSelecao){DeleteObject(bitmapClipboardSelecao);bitmapClipboardSelecao=NULL;}
                if(bitmapPreviewColagemVermelho){DeleteObject(bitmapPreviewColagemVermelho);bitmapPreviewColagemVermelho=NULL;}
                colandoSelecao=false; transformacaoSelecaoAtiva=false; g_pendingImageAssetRelativePath.clear();
                caminhoProjetoAtivo[0]=L'\0';
                for (size_t i = 0; i < listaFrames.size(); ++i) ExcluirBitmapsQuadro(listaFrames[i]);
                InvalidarCacheComposicaoCanvas(); listaFrames.clear();
                timelineLayers.clear(); timelineLayers.push_back({L"Camada 1", true}); activeTimelineLayer=selectedTimelineLayer=0;
                FrameData f;
                f.id = 1;
                f.segundos = 1; f.unidadeDuracao=durationUnit;
                f.hBmpQuadro = CriarBitmapEmBranco(configAtual.largura, configAtual.altura,
                                                   configAtual.corFundo);
                GarantirBitmapsCamadas(f);
                SalvarEstadoHistorico(f);
                listaFrames.push_back(f);
                SalvarProjetoCompleto();
                DestroyWindow(hwnd);
                AbrirJanelasModulares();
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Preenchimento por região em DIB de 32 bits. O nível Gap controla a tolerância
// de cor nas bordas, evitando pequenas diferenças de antialiasing.
void AplicarBaldeComTolerancia(HBITMAP bitmap, int x, int y) {
    if (!bitmap) return;
    BITMAP bm{};
    if (!GetObjectW(bitmap, sizeof(bm), &bm) || bm.bmWidth <= 0 || bm.bmHeight <= 0) return;
    if (x < 0 || y < 0 || x >= bm.bmWidth || y >= bm.bmHeight) return;

    const int w = bm.bmWidth, h = bm.bmHeight;
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // linhas de cima para baixo
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    std::vector<BYTE> pixels((size_t)w * (size_t)h * 4);
    HDC screen = GetDC(NULL);
    if (!screen) return;
    int lines = GetDIBits(screen, bitmap, 0, h, pixels.data(), &bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, screen);
    if (lines == 0) return;

    const size_t seed = ((size_t)y * w + x) * 4;
    const int sb = pixels[seed], sg = pixels[seed+1], sr = pixels[seed+2];
    const int tolerance = std::max(0, std::min(100, nivelGapBalde)) * 2;
    // Marca as linhas/contornos que contrastam com a cor do ponto clicado.
    // Em seguida expande essa barreira alguns pixels para fechar pequenos buracos.
    std::vector<BYTE> bloqueado((size_t)w * h, 0);
    for (int py=0; py<h; ++py) for (int px=0; px<w; ++px) {
        size_t p=((size_t)py*w+px)*4;
        int db=std::abs((int)pixels[p]-sb), dg=std::abs((int)pixels[p+1]-sg), dr=std::abs((int)pixels[p+2]-sr);
        if (std::max(db,std::max(dg,dr)) > std::max(32,tolerance+16)) bloqueado[(size_t)py*w+px]=1;
    }
    int raio=std::max(0,std::min(20,toleranciaBalde));
    if (raio>0) {
        // Dilatação separável: fecha falhas sem o custo quadrático por pixel.
        std::vector<BYTE> horizontal((size_t)w*h, 0);
        for (int py=0; py<h; ++py) for (int px=0; px<w; ++px) {
            int x0=std::max(0,px-raio), x1=std::min(w-1,px+raio);
            for (int xx=x0; xx<=x1; ++xx) if (bloqueado[(size_t)py*w+xx]) { horizontal[(size_t)py*w+px]=1; break; }
        }
        for (int py=0; py<h; ++py) for (int px=0; px<w; ++px) {
            int y0=std::max(0,py-raio), y1=std::min(h-1,py+raio);
            for (int yy=y0; yy<=y1; ++yy) if (horizontal[(size_t)yy*w+px]) { bloqueado[(size_t)py*w+px]=1; break; }
        }
    }
    auto matches = [&](size_t pos) -> bool {
        size_t pixelIndex=pos/4;
        return !bloqueado[pixelIndex] &&
               std::abs((int)pixels[pos] - sb) <= tolerance &&
               std::abs((int)pixels[pos+1] - sg) <= tolerance &&
               std::abs((int)pixels[pos+2] - sr) <= tolerance;
    };
    const BYTE fb = GetBValue(corAtual), fg = GetGValue(corAtual), fr = GetRValue(corAtual);
    if (std::abs(sb-(int)fb) <= 0 && std::abs(sg-(int)fg) <= 0 && std::abs(sr-(int)fr) <= 0) return;

    // Preenchimento scanline: processa faixas horizontais em vez de empilhar
    // cada pixel e manter um vetor visited do tamanho inteiro da imagem.
    // Isso reduz alocações e duplicatas em áreas grandes.
    std::vector<int> stack;
    stack.reserve((size_t)std::min(w * h, 65536));
    stack.push_back(y * w + x);
    while (!stack.empty()) {
        const int seedIdx = stack.back(); stack.pop_back();
        int sy = seedIdx / w;
        int sx = seedIdx % w;
        size_t seedPos = ((size_t)sy * w + sx) * 4;
        if (!matches(seedPos)) continue;
        int left = sx;
        while (left > 0 && matches(((size_t)sy * w + (left - 1)) * 4)) --left;
        bool spanAbove = false, spanBelow = false;
        for (int px = left; px < w; ++px) {
            size_t pos = ((size_t)sy * w + px) * 4;
            if (!matches(pos)) break;
            pixels[pos] = fb; pixels[pos+1] = fg; pixels[pos+2] = fr; pixels[pos+3] = 0;
            if (sy > 0) {
                size_t up = ((size_t)(sy - 1) * w + px) * 4;
                bool m = matches(up);
                if (m && !spanAbove) stack.push_back((sy - 1) * w + px);
                spanAbove = m;
            }
            if (sy + 1 < h) {
                size_t dn = ((size_t)(sy + 1) * w + px) * 4;
                bool m = matches(dn);
                if (m && !spanBelow) stack.push_back((sy + 1) * w + px);
                spanBelow = m;
            }
        }
    }

    screen = GetDC(NULL);
    if (screen) {
        SetDIBits(screen, bitmap, 0, h, pixels.data(), &bi, DIB_RGB_COLORS);
        ReleaseDC(NULL, screen);
    }
}

void LimparDesenhoDoQuadroAtual() {
    if (frameAtual < 0 || frameAtual >= (int)listaFrames.size()) return;
    FrameData& f = listaFrames[frameAtual];
    if (!f.hBmpQuadro) return;
    SalvarEstadoHistorico(f);
    HBITMAP novo = CriarBitmapEmBranco(configAtual.largura, configAtual.altura, RGB(255,255,255));
    if (!novo) return;
    DeleteObject(f.hBmpQuadro);
    f.hBmpQuadro = novo;
    if (!f.layerBitmaps.empty() && activeTimelineLayer >= 0 && activeTimelineLayer < (int)f.layerBitmaps.size()) f.layerBitmaps[activeTimelineLayer] = novo;
    InvalidarCacheComposicaoCanvas(); if (hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
    if (hwndTimeline) InvalidateRect(hwndTimeline, NULL, TRUE);
    SalvarProjetoCompleto();
}

void ExcluirQuadroAtual() {
    if (listaFrames.empty() || frameAtual < 0 || frameAtual >= (int)listaFrames.size()) return;
    RegistrarHistoricoTimeline();
    FrameData& f=listaFrames[frameAtual];
    if(selectedTimelineLayer>=0 && selectedTimelineLayer<(int)timelineLayers.size() && QuadroExisteNaCamada(f,selectedTimelineLayer)) {
        if(f.layerFrameExists.empty()) f.layerFrameExists.assign(timelineLayers.size(),true);
        f.layerFrameExists[selectedTimelineLayer]=false;
        bool layerHasOther=false; for(int fi=0;fi<(int)listaFrames.size();++fi) if(fi!=frameAtual && QuadroExisteNaCamada(listaFrames[fi],selectedTimelineLayer)){layerHasOther=true;break;}
        if(!layerHasOther){
            // Não deixa a camada sem nenhum quadro: o último vira um quadro em branco.
            f.layerFrameExists[selectedTimelineLayer]=true;
            if(selectedTimelineLayer<(int)f.layerBitmaps.size()){
                HBITMAP blank=CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255));
                if(blank){ if(f.layerBitmaps[selectedTimelineLayer]) DeleteObject(f.layerBitmaps[selectedTimelineLayer]); f.layerBitmaps[selectedTimelineLayer]=blank; }
                f.hBmpQuadro=f.layerBitmaps[selectedTimelineLayer];
            }
        } else {
            bool any=false; for(int li=0;li<(int)timelineLayers.size();++li) if(QuadroExisteNaCamada(f,li)){any=true;break;}
            if(!any && listaFrames.size()>1){ ExcluirBitmapsQuadro(f); listaFrames.erase(listaFrames.begin()+frameAtual); }
        }
    } else if(listaFrames.size()>1) { ExcluirBitmapsQuadro(f); listaFrames.erase(listaFrames.begin()+frameAtual); }
    else LimparDesenhoDoQuadroAtual();
    if(frameAtual >= (int)listaFrames.size()) frameAtual=(int)listaFrames.size()-1;
    if(frameAtual<0) frameAtual=0;
    if(!listaFrames.empty() && !QuadroExisteNaCamada(listaFrames[frameAtual],selectedTimelineLayer)) {
        int nearest=-1;
        for(int i=frameAtual-1;i>=0;--i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)){nearest=i;break;}
        if(nearest<0) for(int i=frameAtual+1;i<(int)listaFrames.size();++i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)){nearest=i;break;}
        if(nearest>=0) frameAtual=nearest;
    }
    InvalidarCacheComposicaoCanvas(); if (hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
    if (hwndTimeline) InvalidateRect(hwndTimeline, NULL, TRUE);
    SalvarProjetoCompleto();
}

static int FrameForTimelineX(int x);
void AplicarDeletePelaPosicaoDoMouse() {
    POINT pt{};
    if (!GetCursorPos(&pt)) return;
    HWND under = WindowFromPoint(pt);
    if (!under) return;
    if (hwndCanvas && (under == hwndCanvas || IsChild(hwndCanvas, under))) {
        LimparDesenhoDoQuadroAtual();
        return;
    }
    if (hwndTimeline && (under == hwndTimeline || IsChild(hwndTimeline, under))) {
        POINT local=pt;
        if (!ScreenToClient(hwndTimeline,&local)) return;
        // A área superior contém botões de timeline; não apague quadros nela.
        if (local.y < TIMELINE_RULER_TOP + alturaReguaTimeline) return;
        const int row=(local.y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/std::max(1,alturaLinhaCamada)+timelineLayerScroll;
        if (local.x < TIMELINE_SPLIT_X) {
            // Cabeçalho/linha de camada: seleciona a camada sob a mira e aciona
            // o mesmo comando de exclusão da interface, incluindo todos os quadros.
            if (row>=0 && row<(int)timelineLayers.size()) {
                selectedTimelineLayer=activeTimelineLayer=row;
                SincronizarAliasCamadaAtiva();
                SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_LAYER_DELETE,0),
                             (LPARAM)GetDlgItem(hwndTimeline,ID_LAYER_DELETE));
            }
            return;
        }
        // Coluna de quadro: seleciona a camada/coluna apontada antes de excluir.
        if (row>=0 && row<(int)timelineLayers.size()) {
            selectedTimelineLayer=activeTimelineLayer=row;
            SincronizarAliasCamadaAtiva();
        }
        const int col=FrameForTimelineX(local.x);
        if (col>=0 && col<(int)listaFrames.size()) {
            frameAtual=col;
            selectedTimelineFrame=col;
            ExcluirQuadroAtual();
        }
        return;
    }
}

static HBITMAP BitmapCamadaAtual() {
    if(frameAtual<0 || frameAtual>=(int)listaFrames.size()) return NULL;
    GarantirBitmapsCamadas(listaFrames[frameAtual]);
    if(activeTimelineLayer>=0 && activeTimelineLayer<(int)listaFrames[frameAtual].layerBitmaps.size())
        return listaFrames[frameAtual].layerBitmaps[activeTimelineLayer];
    return listaFrames[frameAtual].hBmpQuadro;
}
static bool ScreenToBitmapPoint(HWND hwnd,HBITMAP hb,POINT p,int& x,int& y) {
    BITMAP bm{}; if(!hb || !GetObjectW(hb,sizeof(bm),&bm)) return false;
    RECT rc{}; GetClientRect(hwnd,&rc);
    double cx=rc.right/2.0+panX, cy=rc.bottom/2.0+panY;
    double sx=p.x-cx, sy=p.y-cy;
    double a=-(double)anguloCanvas*3.14159265358979323846/180.0;
    double ca=cos(a),sa=sin(a), rx=sx*ca-sy*sa, ry=sx*sa+sy*ca;
    x=(int)floor((rx+bm.bmWidth*zoomFactor/2.0)/zoomFactor);
    y=(int)floor((ry+bm.bmHeight*zoomFactor/2.0)/zoomFactor);
    return true;
}
static POINT BitmapToScreenPoint(HWND hwnd,HBITMAP hb,double x,double y) {
    POINT out={0,0}; BITMAP bm{}; RECT rc{};
    if(!hb || !GetObjectW(hb,sizeof(bm),&bm)) return out; GetClientRect(hwnd,&rc);
    double rx=x*zoomFactor-bm.bmWidth*zoomFactor/2.0, ry=y*zoomFactor-bm.bmHeight*zoomFactor/2.0;
    double a=(double)anguloCanvas*3.14159265358979323846/180.0;
    double sx=rx*cos(a)-ry*sin(a), sy=rx*sin(a)+ry*cos(a);
    out.x=(LONG)lround(sx+rc.right/2.0+panX); out.y=(LONG)lround(sy+rc.bottom/2.0+panY); return out;
}
static void AtualizarContornoSelecao(HWND hwnd) {
    HBITMAP base=BitmapCamadaAtual(); if(!base || !bitmapSelecaoRaster)return;
    RECT rc{}; GetClientRect(hwnd,&rc);
    if(modoLaco==1 && pontosLacoBitmap.size()>=3){
        pontosLacoTela.clear();pontosLacoTela.reserve(pontosLacoBitmap.size());
        rectSelecaoTela.left=rc.right;rectSelecaoTela.top=rc.bottom;rectSelecaoTela.right=0;rectSelecaoTela.bottom=0;
        for(const POINT& p:pontosLacoBitmap){
            POINT v=BitmapToScreenPoint(hwnd,base,p.x,p.y);pontosLacoTela.push_back(v);
            rectSelecaoTela.left=std::min(rectSelecaoTela.left,v.x);rectSelecaoTela.top=std::min(rectSelecaoTela.top,v.y);
            rectSelecaoTela.right=std::max(rectSelecaoTela.right,v.x);rectSelecaoTela.bottom=std::max(rectSelecaoTela.bottom,v.y);
        }
    } else {
        pontosLacoTela.clear(); BITMAP bm{};if(!GetObjectW(bitmapSelecaoRaster,sizeof(bm),&bm))return;
        double left=centroSelecaoBmp.x-bm.bmWidth/2.0,top=centroSelecaoBmp.y-bm.bmHeight/2.0;
        POINT v[4]={BitmapToScreenPoint(hwnd,base,left,top),BitmapToScreenPoint(hwnd,base,left+bm.bmWidth,top),
                    BitmapToScreenPoint(hwnd,base,left+bm.bmWidth,top+bm.bmHeight),BitmapToScreenPoint(hwnd,base,left,top+bm.bmHeight)};
        rectSelecaoTela.left=rc.right;rectSelecaoTela.top=rc.bottom;rectSelecaoTela.right=0;rectSelecaoTela.bottom=0;
        for(const POINT& p:v){rectSelecaoTela.left=std::min(rectSelecaoTela.left,p.x);rectSelecaoTela.top=std::min(rectSelecaoTela.top,p.y);rectSelecaoTela.right=std::max(rectSelecaoTela.right,p.x);rectSelecaoTela.bottom=std::max(rectSelecaoTela.bottom,p.y);}
    }
}
static void LiberarCacheRotacaoSelecao() {
    if(bitmapCacheRotacaoSelecao) DeleteObject(bitmapCacheRotacaoSelecao);
    bitmapCacheRotacaoSelecao=NULL; bitmapCacheFonteSelecao=NULL;
    bitmapCacheRotacaoX=bitmapCacheRotacaoY=bitmapCacheRotacaoW=bitmapCacheRotacaoH=0;
    bitmapCacheRotacaoAngulo=1.0e30; bitmapCacheRotacaoEscala=-1.0f;
}
static void LiberarBitmapSelecaoRaster() {
    LiberarCacheRotacaoSelecao();
    if(bitmapSelecaoRaster){DeleteObject(bitmapSelecaoRaster);bitmapSelecaoRaster=NULL;}
    selecaoBmpW=selecaoBmpH=0; pontosLacoBitmap.clear();
}
static void PrepararBackupTransformacao() {
    if(bitmapBackupTransformacao){DeleteObject(bitmapBackupTransformacao);bitmapBackupTransformacao=NULL;}
    backupTransformacaoX=backupTransformacaoY=backupTransformacaoW=backupTransformacaoH=0;
    HBITMAP hb=BitmapCamadaAtual(); BITMAP bm{};
    if(!hb || !GetObjectW(hb,sizeof(bm),&bm)) return;
    // Cache de cancelamento limitado à região selecionada (e à área visível do quadro).
    int x0=std::max(0,selecaoBmpX), y0=std::max(0,selecaoBmpY);
    int x1=std::min((int)bm.bmWidth,selecaoBmpX+selecaoBmpW);
    int y1=std::min((int)bm.bmHeight,selecaoBmpY+selecaoBmpH);
    if(x1<=x0 || y1<=y0) return;
    HDC src=CreateCompatibleDC(NULL), dst=CreateCompatibleDC(NULL);
    if(!src || !dst){if(src)DeleteDC(src);if(dst)DeleteDC(dst);return;}
    HBITMAP os=(HBITMAP)SelectObject(src,hb);
    bitmapBackupTransformacao=CreateCompatibleBitmap(src,x1-x0,y1-y0);
    if(bitmapBackupTransformacao){
        HBITMAP od=(HBITMAP)SelectObject(dst,bitmapBackupTransformacao);
        BitBlt(dst,0,0,x1-x0,y1-y0,src,x0,y0,SRCCOPY);
        SelectObject(dst,od); backupTransformacaoX=x0; backupTransformacaoY=y0;
        backupTransformacaoW=x1-x0; backupTransformacaoH=y1-y0;
    }
    SelectObject(src,os);DeleteDC(src);DeleteDC(dst);
}
static void RestaurarBackupTransformacao() {
    if(!bitmapBackupTransformacao)return; HBITMAP dstBmp=BitmapCamadaAtual();
    if(dstBmp && backupTransformacaoW>0 && backupTransformacaoH>0){
        HDC dd=CreateCompatibleDC(NULL),ss=CreateCompatibleDC(NULL);
        if(dd && ss){
            HBITMAP od=(HBITMAP)SelectObject(dd,dstBmp),os=(HBITMAP)SelectObject(ss,bitmapBackupTransformacao);
            BitBlt(dd,backupTransformacaoX,backupTransformacaoY,backupTransformacaoW,backupTransformacaoH,ss,0,0,SRCCOPY);
            SelectObject(dd,od);SelectObject(ss,os);
        }
        if(dd)DeleteDC(dd); if(ss)DeleteDC(ss);
    }
    DeleteObject(bitmapBackupTransformacao);bitmapBackupTransformacao=NULL;
    backupTransformacaoX=backupTransformacaoY=backupTransformacaoW=backupTransformacaoH=0;
    InvalidarCacheComposicaoCanvas();
}
static bool CapturarSelecaoRaster(HWND hwnd) {
    HBITMAP src=BitmapCamadaAtual(); BITMAP bm{};
    if(!src || !GetObjectW(src,sizeof(bm),&bm)) return false;

    // Krita-style: conserva o caminho de entrada, calcula os limites no espaço da
    // imagem e rasteriza a máscara de seleção uma única vez, sem SetPixel por pixel.
    std::vector<POINT> screenPath;
    if(modoLaco==1 && pontosLacoTela.size()>=3) {
        screenPath=pontosLacoTela;
    } else {
        screenPath.push_back(POINT{rectSelecaoTela.left,rectSelecaoTela.top});
        screenPath.push_back(POINT{rectSelecaoTela.right,rectSelecaoTela.top});
        screenPath.push_back(POINT{rectSelecaoTela.right,rectSelecaoTela.bottom});
        screenPath.push_back(POINT{rectSelecaoTela.left,rectSelecaoTela.bottom});
    }
    if(screenPath.size()<3) return false;
    std::vector<POINT> pathBmp; pathBmp.reserve(screenPath.size());
    int minx=(int)bm.bmWidth, miny=(int)bm.bmHeight, maxx=0, maxy=0;
    for(const POINT& p:screenPath){
        int x=0,y=0; if(!ScreenToBitmapPoint(hwnd,src,p,x,y)) return false;
        pathBmp.push_back(POINT{x,y});
        minx=std::min(minx,x); miny=std::min(miny,y);
        maxx=std::max(maxx,x); maxy=std::max(maxy,y);
    }
    minx=std::max(0,std::min((int)bm.bmWidth,minx));
    miny=std::max(0,std::min((int)bm.bmHeight,miny));
    maxx=std::max(0,std::min((int)bm.bmWidth,maxx+1));
    maxy=std::max(0,std::min((int)bm.bmHeight,maxy+1));
    if(maxx<=minx || maxy<=miny) return false;
    const int sw=maxx-minx, sh=maxy-miny;
    std::vector<POINT> localPath; localPath.reserve(pathBmp.size());
    for(const POINT& p:pathBmp) localPath.push_back(POINT{p.x-minx,p.y-miny});

    LiberarBitmapSelecaoRaster();
    pontosLacoBitmap=pathBmp;
    selecaoBmpX=minx; selecaoBmpY=miny; selecaoBmpW=sw; selecaoBmpH=sh;
    HDC sd=CreateCompatibleDC(NULL), dd=CreateCompatibleDC(NULL);
    if(!sd || !dd){if(sd)DeleteDC(sd);if(dd)DeleteDC(dd);LiberarBitmapSelecaoRaster();return false;}
    HBITMAP oldS=(HBITMAP)SelectObject(sd,src);
    bitmapSelecaoRaster=CreateCompatibleBitmap(sd,sw,sh);
    if(!bitmapSelecaoRaster){SelectObject(sd,oldS);DeleteDC(sd);DeleteDC(dd);LiberarBitmapSelecaoRaster();return false;}
    HBITMAP oldD=(HBITMAP)SelectObject(dd,bitmapSelecaoRaster);
    if(!BitBlt(dd,0,0,sw,sh,sd,minx,miny,SRCCOPY)){
        SelectObject(dd,oldD);SelectObject(sd,oldS);DeleteDC(dd);DeleteDC(sd);LiberarBitmapSelecaoRaster();return false;
    }
    if(localPath.size()>=3 && localPath.size()<(size_t)INT_MAX){
        HRGN inside=CreatePolygonRgn(localPath.data(),(int)localPath.size(),ALTERNATE);
        HRGN outside=CreateRectRgn(0,0,sw,sh);
        if(inside && outside){
            CombineRgn(outside,outside,inside,RGN_DIFF);
            HBRUSH bg=CreateSolidBrush(configAtual.corFundo);
            FillRgn(dd,outside,bg);
            DeleteObject(bg);
        }
        if(inside)DeleteObject(inside); if(outside)DeleteObject(outside);
    }
    SelectObject(dd,oldD);SelectObject(sd,oldS);DeleteDC(dd);DeleteDC(sd);
    centroSelecaoBmp.x=selecaoBmpX+selecaoBmpW/2; centroSelecaoBmp.y=selecaoBmpY+selecaoBmpH/2;
    escalaSelecaoRaster=1.0f; anguloSelecaoRaster=0.0;
    return true;
}
static void LimparRetanguloOrigemSelecao() {
    HBITMAP hb=BitmapCamadaAtual(); if(!hb)return;
    HDC dc=CreateCompatibleDC(NULL); if(!dc)return;
    HBITMAP old=(HBITMAP)SelectObject(dc,hb);
    HBRUSH br=CreateSolidBrush(configAtual.corFundo);
    if(br && pontosLacoBitmap.size()>=3 && pontosLacoBitmap.size()<(size_t)INT_MAX){
        HRGN region=CreatePolygonRgn(pontosLacoBitmap.data(),(int)pontosLacoBitmap.size(),ALTERNATE);
        if(region){FillRgn(dc,region,br);DeleteObject(region);}
    } else {
        RECT r={selecaoBmpX,selecaoBmpY,selecaoBmpX+selecaoBmpW,selecaoBmpY+selecaoBmpH};
        FillRect(dc,&r,br);
    }
    if(br)DeleteObject(br); SelectObject(dc,old);DeleteDC(dc);
    InvalidarCacheComposicaoCanvas();
}
static bool GarantirCacheRotacaoSelecao() {
    if(!bitmapSelecaoRaster) return false;
    if(bitmapCacheRotacaoSelecao && bitmapCacheFonteSelecao==bitmapSelecaoRaster &&
       std::fabs(bitmapCacheRotacaoAngulo-anguloSelecaoRaster)<0.0001 &&
       std::fabs(bitmapCacheRotacaoEscala-escalaSelecaoRaster)<0.0001f) return true;
    LiberarCacheRotacaoSelecao();
    BITMAP bm{}; if(!GetObjectW(bitmapSelecaoRaster,sizeof(bm),&bm) || bm.bmWidth<=0 || bm.bmHeight<=0) return false;
    const double scale=std::max(0.05,std::min(20.0,(double)escalaSelecaoRaster));
    const double w=(double)bm.bmWidth*scale, h=(double)bm.bmHeight*scale;
    const double a=anguloSelecaoRaster*3.14159265358979323846/180.0;
    const double ca=std::cos(a), sa=std::sin(a);
    const int outW=(int)std::ceil(std::fabs(w*ca)+std::fabs(h*sa))+2;
    const int outH=(int)std::ceil(std::fabs(w*sa)+std::fabs(h*ca))+2;
    if(outW<=0 || outH<=0 || outW>32768 || outH>32768) return false;
    size_t maxBytes=256u*1024u*1024u;
    if(limiteRAMMB>0) maxBytes=std::min(maxBytes,(size_t)limiteRAMMB*1024u*1024u/4u);
    const size_t pixels=(size_t)outW*(size_t)outH;
    if(pixels>maxBytes/4u || pixels>((size_t)-1)/4u) return false;
    HDC screen=GetDC(hwndCanvas ? hwndCanvas : NULL); if(!screen)return false;
    HBITMAP cache=CreateCompatibleBitmap(screen,outW,outH);
    if(!cache){ReleaseDC(hwndCanvas ? hwndCanvas : NULL,screen);return false;}
    HDC dst=CreateCompatibleDC(screen), src=CreateCompatibleDC(screen);
    if(!dst || !src){if(dst)DeleteDC(dst);if(src)DeleteDC(src);DeleteObject(cache);ReleaseDC(hwndCanvas ? hwndCanvas : NULL,screen);return false;}
    HBITMAP oldD=(HBITMAP)SelectObject(dst,cache), oldS=(HBITMAP)SelectObject(src,bitmapSelecaoRaster);
    HBRUSH bg=CreateSolidBrush(configAtual.corFundo); RECT all={0,0,outW,outH}; FillRect(dst,&all,bg); DeleteObject(bg);
    POINT q[3]; const double cx=outW/2.0, cy=outH/2.0;
    q[0]=POINT{(LONG)std::lround(cx-w*ca/2.0+h*sa/2.0),(LONG)std::lround(cy-w*sa/2.0-h*ca/2.0)};
    q[1]=POINT{(LONG)std::lround(cx+w*ca/2.0+h*sa/2.0),(LONG)std::lround(cy+w*sa/2.0-h*ca/2.0)};
    q[2]=POINT{(LONG)std::lround(cx-w*ca/2.0-h*sa/2.0),(LONG)std::lround(cy-w*sa/2.0+h*ca/2.0)};
    SetStretchBltMode(dst,HALFTONE); SetBrushOrgEx(dst,0,0,NULL);
    BOOL ok=PlgBlt(dst,q,src,0,0,bm.bmWidth,bm.bmHeight,NULL,0,0);
    SelectObject(dst,oldD);SelectObject(src,oldS);DeleteDC(dst);DeleteDC(src);ReleaseDC(hwndCanvas ? hwndCanvas : NULL,screen);
    if(!ok){DeleteObject(cache);return false;}
    bitmapCacheRotacaoSelecao=cache; bitmapCacheFonteSelecao=bitmapSelecaoRaster;
    bitmapCacheRotacaoW=outW; bitmapCacheRotacaoH=outH;
    bitmapCacheRotacaoX=(int)std::floor(centroSelecaoBmp.x-outW/2.0);
    bitmapCacheRotacaoY=(int)std::floor(centroSelecaoBmp.y-outH/2.0);
    bitmapCacheRotacaoAngulo=anguloSelecaoRaster; bitmapCacheRotacaoEscala=escalaSelecaoRaster;
    return true;
}
static void CopiarSelecaoInterna(bool cortar) {
    if(!selecaoAtiva || !bitmapSelecaoRaster)return;
    HBITMAP novaCopia=CopiarHBITMAP(bitmapSelecaoRaster,selecaoBmpW,selecaoBmpH);
    if(!novaCopia)return;
    if(bitmapClipboardSelecao)DeleteObject(bitmapClipboardSelecao);
    bitmapClipboardSelecao=novaCopia;
    if(cortar && frameAtual>=0 && frameAtual<(int)listaFrames.size()){
        SalvarEstadoHistorico(listaFrames[frameAtual]); LimparRetanguloOrigemSelecao();
        InvalidarCacheComposicaoCanvas();
    }
    // O botão Copiar e o atalho Ctrl+C encerram a seleção visual, sem apagar a arte original.
    selecaoAtiva=false; lacoAtivo=false; criandoSelecao=false; pontosLacoTela.clear();
    LiberarBitmapSelecaoRaster();
    InvalidarCacheComposicaoCanvas(); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE);
}
typedef BOOL (WINAPI *KOD_TRANSPARENTBLT_FN)(HDC,int,int,int,int,HDC,int,int,int,int,UINT);
static BOOL TransparentBltCompat(HDC dst,int dx,int dy,int dw,int dh,HDC src,int sx,int sy,int sw,int sh,COLORREF key) {
    HMODULE mod=GetModuleHandleW(L"msimg32.dll"); if(!mod) mod=LoadLibraryW(L"msimg32.dll");
    if(!mod) return FALSE; KOD_TRANSPARENTBLT_FN fn=(KOD_TRANSPARENTBLT_FN)GetProcAddress(mod,"TransparentBlt");
    return fn?fn(dst,dx,dy,dw,dh,src,sx,sy,sw,sh,(UINT)key):FALSE;
}
static void ConfirmarTransformacaoSelecao(HWND hwnd) {
    HBITMAP dstBmp=BitmapCamadaAtual(); if(!dstBmp || !bitmapSelecaoRaster)return;
    const POINT centroAntigo=centroSelecaoBmp;
    const double escalaAntiga=std::max(0.05,std::min(20.0,(double)escalaSelecaoRaster));
    const double anguloRad=anguloSelecaoRaster*3.14159265358979323846/180.0;
    std::vector<POINT> caminhoTransformado; caminhoTransformado.reserve(pontosLacoBitmap.size());
    for(const POINT& p:pontosLacoBitmap){
        const double dx=(p.x-centroAntigo.x)*escalaAntiga, dy=(p.y-centroAntigo.y)*escalaAntiga;
        caminhoTransformado.push_back(POINT{0,0});
        caminhoTransformado.back().x=(LONG)std::lround(centroAntigo.x+dx*std::cos(anguloRad)-dy*std::sin(anguloRad));
        caminhoTransformado.back().y=(LONG)std::lround(centroAntigo.y+dx*std::sin(anguloRad)+dy*std::cos(anguloRad));
    }
    HDC dc=CreateCompatibleDC(NULL); if(!dc)return;
    HBITMAP oldD=(HBITMAP)SelectObject(dc,dstBmp);
    HBITMAP result=NULL; int outX=0,outY=0,outW=0,outH=0;

    // O cache é limitado ao objeto selecionado e reutilizado tanto na prévia
    // quanto na confirmação. Isso evita rasterizar a rotação duas vezes.
    if(GarantirCacheRotacaoSelecao()) {
        HBITMAP candidate=bitmapCacheRotacaoSelecao;
        outX=bitmapCacheRotacaoX; outY=bitmapCacheRotacaoY;
        outW=bitmapCacheRotacaoW; outH=bitmapCacheRotacaoH;
        bool drew=false; HDC ss=CreateCompatibleDC(NULL);
        if(ss){HBITMAP os=(HBITMAP)SelectObject(ss,candidate);
            drew=TransparentBltCompat(dc,outX,outY,outW,outH,ss,0,0,outW,outH,configAtual.corFundo)!=FALSE;
            SelectObject(ss,os);DeleteDC(ss);
        }
        if(drew){
            result=candidate; bitmapCacheRotacaoSelecao=NULL; bitmapCacheFonteSelecao=NULL;
            bitmapCacheRotacaoW=bitmapCacheRotacaoH=0; bitmapCacheRotacaoX=bitmapCacheRotacaoY=0;
            bitmapCacheRotacaoAngulo=1.0e30; bitmapCacheRotacaoEscala=-1.0f;
        }
    } else {
        double a=anguloSelecaoRaster*3.14159265358979323846/180.0;
        double w=selecaoBmpW*escalaSelecaoRaster,h=selecaoBmpH*escalaSelecaoRaster;
        double cx=centroSelecaoBmp.x,cy=centroSelecaoBmp.y;
        // Inclui os quatro cantos; usar apenas três cortava a extremidade
        // inferior/direita quando o retângulo era girado.
        POINT q[4]; const double vx[4]={-w/2.0,w/2.0,w/2.0,-w/2.0};
        const double vy[4]={-h/2.0,-h/2.0,h/2.0,h/2.0};
        int minx=INT_MAX,miny=INT_MAX,maxx=INT_MIN,maxy=INT_MIN;
        for(int i=0;i<4;++i){q[i].x=(LONG)std::lround(cx+vx[i]*std::cos(a)-vy[i]*std::sin(a));q[i].y=(LONG)std::lround(cy+vx[i]*std::sin(a)+vy[i]*std::cos(a));minx=std::min(minx,(int)q[i].x);maxx=std::max(maxx,(int)q[i].x);miny=std::min(miny,(int)q[i].y);maxy=std::max(maxy,(int)q[i].y);}
        outX=minx-1;outY=miny-1;outW=std::max(1,maxx-minx+2);outH=std::max(1,maxy-miny+2);
        if(outW>0 && outH>0 && outW<=32768 && outH<=32768 && (size_t)outW*(size_t)outH<=(256u*1024u*1024u)/4u){
            HBITMAP tmp=CreateCompatibleBitmap(dc,outW,outH); HDC td=CreateCompatibleDC(dc);
            if(tmp && td){HBITMAP ot=(HBITMAP)SelectObject(td,tmp);HBRUSH bg=CreateSolidBrush(configAtual.corFundo);RECT tr={0,0,outW,outH};FillRect(td,&tr,bg);DeleteObject(bg);
                POINT tq[3]={{q[0].x-outX,q[0].y-outY},{q[1].x-outX,q[1].y-outY},{q[3].x-outX,q[3].y-outY}};
                bool rotated=false; HDC sd=CreateCompatibleDC(NULL);
                if(sd){HBITMAP os=(HBITMAP)SelectObject(sd,bitmapSelecaoRaster);SetStretchBltMode(td,HALFTONE);SetBrushOrgEx(td,0,0,NULL);rotated=PlgBlt(td,tq,sd,0,0,selecaoBmpW,selecaoBmpH,NULL,0,0)!=FALSE;SelectObject(sd,os);DeleteDC(sd);}
                SelectObject(td,ot);DeleteDC(td);
                if(rotated){HDC ss=CreateCompatibleDC(NULL);if(ss){HBITMAP os=(HBITMAP)SelectObject(ss,tmp);bool drawn=TransparentBltCompat(dc,outX,outY,outW,outH,ss,0,0,outW,outH,configAtual.corFundo)!=FALSE;SelectObject(ss,os);DeleteDC(ss);if(drawn)result=tmp;else DeleteObject(tmp);}else DeleteObject(tmp);}
                else DeleteObject(tmp);
            } else {if(tmp)DeleteObject(tmp);if(td)DeleteDC(td);}
        }
    }
    SelectObject(dc,oldD);DeleteDC(dc);
    if(!result){
        MessageBoxW(hwnd,L"Não foi possível reservar memória suficiente para girar este objeto. Reduza a escala ou cancele a transformação para restaurar a arte original.",L"Kodanima - Rotação",MB_OK|MB_ICONWARNING);
        return;
    }
    if(result){
        if(bitmapSelecaoRaster)DeleteObject(bitmapSelecaoRaster);
        bitmapSelecaoRaster=result; selecaoBmpX=outX;selecaoBmpY=outY;selecaoBmpW=outW;selecaoBmpH=outH;
        centroSelecaoBmp.x=outX+outW/2;centroSelecaoBmp.y=outY+outH/2;
        // Preserva o contorno da máscara na nova posição para transformações
        // subsequentes; não volta a apagar todo o retângulo do objeto.
        if(!caminhoTransformado.empty()){
            const double shiftX=(double)centroSelecaoBmp.x-centroAntigo.x;
            const double shiftY=(double)centroSelecaoBmp.y-centroAntigo.y;
            for(POINT& q:caminhoTransformado){q.x=(LONG)std::lround(q.x+shiftX);q.y=(LONG)std::lround(q.y+shiftY);}
            pontosLacoBitmap.swap(caminhoTransformado);
        }
        if(bitmapCacheRotacaoSelecao)LiberarCacheRotacaoSelecao();
    }
    if(bitmapBackupTransformacao){DeleteObject(bitmapBackupTransformacao);bitmapBackupTransformacao=NULL;}
    backupTransformacaoX=backupTransformacaoY=backupTransformacaoW=backupTransformacaoH=0;
    escalaSelecaoRaster=1.0f;anguloSelecaoRaster=0.0;transformacaoSelecaoAtiva=false;transformacaoRotacao=false;transformacaoEscalaForcada=false;transformacaoMoverSelecao=false;selecaoMoverRestoX=selecaoMoverRestoY=0.0;operacaoSelecao=0;
    AtualizarContornoSelecao(hwnd);
    transformacaoMoverSelecao=false;selecaoMoverRestoX=selecaoMoverRestoY=0.0;
    InvalidarCacheComposicaoCanvas();InvalidateRect(hwnd,NULL,FALSE);
}
static HBITMAP CriarPreviewVermelho(HBITMAP source) {
    if(!source)return NULL;BITMAP bm{};if(!GetObjectW(source,sizeof(bm),&bm))return NULL;
    HDC sd=CreateCompatibleDC(NULL);HBITMAP os=(HBITMAP)SelectObject(sd,source);HBITMAP out=CreateCompatibleBitmap(sd,bm.bmWidth,bm.bmHeight);HDC dd=CreateCompatibleDC(sd);HBITMAP od=(HBITMAP)SelectObject(dd,out);
    for(int y=0;y<bm.bmHeight;y++)for(int x=0;x<bm.bmWidth;x++){COLORREF c=GetPixel(sd,x,y);SetPixel(dd,x,y,(c==configAtual.corFundo)?configAtual.corFundo:RGB(255,35,45));}
    SelectObject(dd,od);DeleteDC(dd);SelectObject(sd,os);DeleteDC(sd);return out;
}
static void ConfirmarColagemInterna(HWND hwnd) {
    if(!colandoSelecao || !bitmapClipboardSelecao)return;
    HBITMAP dstBmp=BitmapCamadaAtual(); if(!dstBmp)return;
    BITMAP bm{}; if(!GetObjectW(bitmapClipboardSelecao,sizeof(bm),&bm))return;
    HDC dd=CreateCompatibleDC(NULL), ss=CreateCompatibleDC(NULL);
    if(!dd || !ss){if(dd)DeleteDC(dd);if(ss)DeleteDC(ss);return;}
    HBITMAP od=(HBITMAP)SelectObject(dd,dstBmp), os=(HBITMAP)SelectObject(ss,bitmapClipboardSelecao);
    double w=bm.bmWidth*colagemEscala,h=bm.bmHeight*colagemEscala;
    double cx=cursorColagemBmp.x+bm.bmWidth/2.0,cy=cursorColagemBmp.y+bm.bmHeight/2.0;
    double a=colagemAngulo*3.14159265358979323846/180.0;
    // Quatro cantos para calcular o limite completo e não cortar o canto
    // inferior/direito ao rodar um objeto colado.
    POINT q[4]; const double vx[4]={-w/2.0,w/2.0,w/2.0,-w/2.0};
    const double vy[4]={-h/2.0,-h/2.0,h/2.0,h/2.0};
    int minx=INT_MAX,miny=INT_MAX,maxx=INT_MIN,maxy=INT_MIN;
    for(int i=0;i<4;++i){
        q[i].x=(LONG)std::lround(cx+vx[i]*std::cos(a)-vy[i]*std::sin(a));
        q[i].y=(LONG)std::lround(cy+vx[i]*std::sin(a)+vy[i]*std::cos(a));
        minx=std::min(minx,(int)q[i].x);maxx=std::max(maxx,(int)q[i].x);
        miny=std::min(miny,(int)q[i].y);maxy=std::max(maxy,(int)q[i].y);
    }
    const int outX=minx-1,outY=miny-1;
    const int tw=std::max(1,maxx-minx+2),th=std::max(1,maxy-miny+2);
    size_t maxBytes=256u*1024u*1024u;
    if(limiteRAMMB>0)maxBytes=std::min(maxBytes,(size_t)limiteRAMMB*1024u*1024u/4u);
    const size_t pixels=(size_t)tw*(size_t)th;
    if(tw>32768 || th>32768 || pixels>maxBytes/4u || pixels>((size_t)-1)/4u){
        SelectObject(dd,od);SelectObject(ss,os);DeleteDC(dd);DeleteDC(ss);
        MessageBoxW(hwnd,L"A transformação precisa de mais memória do que o limite permitido. Reduza a escala da colagem.",L"Kodanima - Colagem",MB_OK|MB_ICONWARNING);
        return;
    }
    HBITMAP tmp=CreateCompatibleBitmap(dd,tw,th);HDC td=tmp?CreateCompatibleDC(dd):NULL;
    if(!tmp || !td){if(tmp)DeleteObject(tmp);if(td)DeleteDC(td);SelectObject(dd,od);SelectObject(ss,os);DeleteDC(dd);DeleteDC(ss);return;}
    HBITMAP ot=(HBITMAP)SelectObject(td,tmp);
    HBRUSH back=CreateSolidBrush(configAtual.corFundo);RECT rr={0,0,tw,th};FillRect(td,&rr,back);DeleteObject(back);
    POINT tq[3]={{q[0].x-outX,q[0].y-outY},{q[1].x-outX,q[1].y-outY},{q[3].x-outX,q[3].y-outY}};
    SetStretchBltMode(td,HALFTONE);SetBrushOrgEx(td,0,0,NULL);
    BOOL rotated=PlgBlt(td,tq,ss,0,0,bm.bmWidth,bm.bmHeight,NULL,0,0);
    SelectObject(td,ot);DeleteDC(td);
    if(!rotated){DeleteObject(tmp);SelectObject(dd,od);SelectObject(ss,os);DeleteDC(dd);DeleteDC(ss);return;}
    HDC tempSrc=CreateCompatibleDC(NULL);
    BOOL pasted=FALSE;
    if(tempSrc){
        HBITMAP otSrc=(HBITMAP)SelectObject(tempSrc,tmp);
        if(frameAtual>=0 && frameAtual<(int)listaFrames.size()) SalvarEstadoHistorico(listaFrames[frameAtual]);
        pasted=TransparentBltCompat(dd,outX,outY,tw,th,tempSrc,0,0,tw,th,configAtual.corFundo);
        SelectObject(tempSrc,otSrc);DeleteDC(tempSrc);
    }
    DeleteObject(tmp);
    SelectObject(dd,od);SelectObject(ss,os);DeleteDC(dd);DeleteDC(ss);
    if(!pasted)return;
    if(!g_pendingImageAssetRelativePath.empty()) {
        for(auto& asset:g_projectMediaAssets) if(asset.type==L"image" && asset.relativePath==g_pendingImageAssetRelativePath) {
            asset.frame=std::max(0,frameAtual); asset.layer=std::max(0,selectedTimelineLayer); asset.x=outX; asset.y=outY; asset.startSeconds=timelinePlayheadSeconds; break;
        }
        g_pendingImageAssetRelativePath.clear();
    }
    colandoSelecao=false;colagemModoRotacao=false;colagemModoEscala=false;colagemEscala=1.0f;colagemAngulo=0.0;
    if(bitmapPreviewColagemVermelho){DeleteObject(bitmapPreviewColagemVermelho);bitmapPreviewColagemVermelho=NULL;}
    InvalidarCacheComposicaoCanvas();InvalidateRect(hwnd,NULL,FALSE);
    SalvarProjetoCompleto();
}

static bool MouseMessagePromovidaDeCaneta() {
    // Windows tablet/Ink may promote the same pen input to WM_MOUSE messages.
    // Wintab is the selected source in mode 0, so ignore only pen-promoted mouse
    // messages; a physical mouse remains usable for pan/zoom.
    const ULONG_PTR extra = (ULONG_PTR)GetMessageExtraInfo();
    const ULONG_PTR signature = (ULONG_PTR)0xFF515700u;
    return ((extra & (ULONG_PTR)0xFFFFFF00u) == signature) && ((extra & 0x80u) == 0);
}

static void ProcessarPontoCaneta(HWND hwnd, POINT clientPoint, float pressure, bool contact) {
    pressure = std::max(0.0f, std::min(1.0f, pressure));

    // Sincroniza o cursor real do Windows com a posição reportada pela caneta.
    // Isso permite que o cursor escolhido pelo usuário (.cur, .ani ou padrão)
    // acompanhe a ponta também quando o driver Wintab não move o cursor do sistema.
    if (hwnd && IsWindow(hwnd) && pressureApi != 0) {
        // Não reposiciona o cursor a cada pacote Wintab: CXO_SYSTEM já sincroniza
        // o cursor do sistema. Evita chamadas redundantes que deixam o traço lento.
        POINT screenPoint = clientPoint;
        if (ClientToScreen(hwnd, &screenPoint)) {
            SetCursorPos(screenPoint.x, screenPoint.y);
            SetCursor(cursorPersonalizado ? cursorPersonalizado
                                          : LoadCursorW(NULL, MAKEINTRESOURCEW(32515)));
        }
    }

    const LPARAM lp = MAKELPARAM((SHORT)clientPoint.x, (SHORT)clientPoint.y);
    const bool oldSynthetic = eventoCanetaSintetico;
    eventoCanetaSintetico = true;

    if (contact && !canetaPointerEmContato) {
        canetaPointerEmContato = true;
        pressaoAnteriorCaneta = pressure;
        pressaoAtualCaneta = pressure;
        WndProcCanvas(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
    } else if (contact && canetaPointerEmContato) {
        pressaoAnteriorCaneta = pressaoAtualCaneta;
        pressaoAtualCaneta = pressure;
        WndProcCanvas(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp);
    } else if (!contact && canetaPointerEmContato) {
        WndProcCanvas(hwnd, WM_LBUTTONUP, 0, lp);
        canetaPointerEmContato = false;
        pressaoAtualCaneta = 0.0f;
    }

    eventoCanetaSintetico = oldSynthetic;
}

static bool WintabMapearParaCliente(HWND hwnd, const KOD_WT_PACKET& packet, POINT* out) {
    if (!out) return false;
    const KOD_LOGCONTEXTA& lc=g_wintabContextConfig;
    long long width=(long long)lc.lcOutExtX; if(width<0) width=-width;
    long long height=(long long)lc.lcOutExtY; if(height<0) height=-height;
    if(width==0) { width=(long long)lc.lcInExtX; if(width<0) width=-width; }
    if(height==0) { height=(long long)lc.lcInExtY; if(height<0) height=-height; }
    int sysOrgX=lc.lcSysOrgX, sysOrgY=lc.lcSysOrgY;
    long long screenW=(long long)lc.lcSysExtX; if(screenW<0) screenW=-screenW;
    long long screenH=(long long)lc.lcSysExtY; if(screenH<0) screenH=-screenH;
    if(screenW==0) { sysOrgX=GetSystemMetrics(SM_XVIRTUALSCREEN); screenW=GetSystemMetrics(SM_CXVIRTUALSCREEN); }
    if(screenH==0) { sysOrgY=GetSystemMetrics(SM_YVIRTUALSCREEN); screenH=GetSystemMetrics(SM_CYVIRTUALSCREEN); }
    if(width==0 || height==0 || screenW==0 || screenH==0) return false;
    long long sx=(long long)sysOrgX + ((long long)(packet.pkX-lc.lcOutOrgX)*screenW)/width;
    // Krita configura lcOutExtY negativo para inverter o eixo Y e escala pela magnitude.
    long long sy=(long long)sysOrgY + ((long long)(packet.pkY-lc.lcOutOrgY)*screenH)/height;
    POINT pt={(LONG)sx,(LONG)sy};
    if (!ScreenToClient(hwnd,&pt)) return false;
    *out=pt;
    return true;
}

static bool WintabJanelaEhFilhaDe(HWND janela, HWND raiz) {
    // So atravessa WS_CHILD; GetParent tambem devolve o owner de popups/dialogos,
    // que nao devem ser confundidos com a superficie do canvas.
    for (HWND atual=janela; atual; ) {
        if (atual==raiz) return true;
        if (!(GetWindowLongPtrW(atual,GWL_STYLE) & WS_CHILD)) break;
        atual=GetParent(atual);
    }
    return false;
}

static bool WintabEnviarMouseInput(DWORD flags) {
    INPUT in{};
    in.type=INPUT_MOUSE;
    in.mi.dwFlags=flags;
    in.mi.dwExtraInfo=(ULONG_PTR)0x4B4F4457u; // assinatura interna "KODW"
    return SendInput(1,&in,sizeof(in))==1;
}

static void WintabFallbackMouse(HWND target, UINT message, WPARAM buttons, POINT screenPoint) {
    if (!target || !IsWindow(target)) return;
    POINT local=screenPoint;
    ScreenToClient(target,&local);
    SendMessageW(target,message,buttons,MAKELPARAM((SHORT)local.x,(SHORT)local.y));
}

static void WintabIniciarCliqueForaCanvas(POINT screenPoint) {
    g_wintabExternalMouseActive=true;
    g_wintabFallbackMouseTarget=WindowFromPoint(screenPoint);
    if (WintabEnviarMouseInput(MOUSEEVENTF_LEFTDOWN)) {
        g_wintabMouseInjectedDown=true;
        g_wintabFallbackMouseTarget=NULL; // SendInput deixa o Windows cuidar da captura
    } else {
        g_wintabMouseInjectedDown=false;
        if (g_wintabFallbackMouseTarget)
            WintabFallbackMouse(g_wintabFallbackMouseTarget,WM_LBUTTONDOWN,MK_LBUTTON,screenPoint);
    }
}

static void WintabAtualizarCliqueForaCanvas(POINT screenPoint) {
    // SetCursorPos fornece movimentos normais do sistema enquanto SendInput
    // mantem o botao esquerdo pressionado. So o fallback manual precisa de MOVE.
    if (g_wintabExternalMouseActive && !g_wintabMouseInjectedDown && g_wintabFallbackMouseTarget)
        WintabFallbackMouse(g_wintabFallbackMouseTarget,WM_MOUSEMOVE,MK_LBUTTON,screenPoint);
}

static void WintabSoltarCliqueForaCanvas(POINT screenPoint) {
    if (!g_wintabExternalMouseActive) return;
    if (g_wintabMouseInjectedDown) {
        WintabEnviarMouseInput(MOUSEEVENTF_LEFTUP);
    } else if (g_wintabFallbackMouseTarget) {
        WintabFallbackMouse(g_wintabFallbackMouseTarget,WM_LBUTTONUP,0,screenPoint);
    }
    g_wintabMouseInjectedDown=false;
    g_wintabExternalMouseActive=false;
    g_wintabFallbackMouseTarget=NULL;
}

static void ProcessarPacoteWintab(HWND hwnd, const KOD_WT_PACKET& packet) {
    LONG pMin=g_wintabPressureAxisAvailable ? g_wintabPressureMin : 0;
    LONG pMax=g_wintabPressureAxisAvailable ? g_wintabPressureMax : 1023;
    if(!g_wintabPressureAxisAvailable) {
        int fallbackLevels=g_tabletManualPressureLevels>=2?g_tabletManualPressureLevels:g_tabletPressureLevelsDetected;
        if(fallbackLevels>=2 && fallbackLevels<=65536) pMax=(LONG)(fallbackLevels-1);
    }
    if (pMax<=pMin) { pMin=0; pMax=1023; }
    const LONG raw=(LONG)packet.pkNormalPressure;
    const LONG clamped=std::max(pMin,std::min(pMax,raw));
    float pressure=(float)(clamped-pMin)/(float)(pMax-pMin);
    pressure=std::max(0.0f,std::min(1.0f,pressure));
    pressure=AjustarPressaoPelosNiveis(pressure,(int)std::min<long long>(65536LL,(long long)pMax-(long long)pMin+1LL));

    // Detecta transições de PK_BUTTONS e também aceita pressão positiva mínima do eixo,
    // preservando os primeiros níveis para que a espessura mínima seja alcançável.
    const DWORD newButtons=packet.pkButtons;
    const DWORD oldButtons=g_wintabButtonsInitialized ? g_wintabLastButtons : newButtons;
    g_wintabLastButtons=newButtons;
    g_wintabButtonsInitialized=true;
    const bool tipButtonDown=(newButtons & 0x00000001u)!=0;
    const bool tipButtonJustReleased=((oldButtons & 0x00000001u)!=0 && !tipButtonDown);
    const LONG pressMark=0;   // aceita o primeiro nível real acima do mínimo do eixo
    const LONG releaseMark=0; // não descarta os níveis baixos, que geram traços finos
    const bool pressureDown=g_wintabContactActive ? clamped>(pMin+releaseMark) : clamped>(pMin+pressMark);
    bool contact=tipButtonDown || pressureDown;
    if (tipButtonJustReleased && !pressureDown) contact=false;
    if (!tipButtonDown && newButtons!=0 && clamped<=pMin+releaseMark) contact=false;
    const bool wasContact=g_wintabContactActive;

    POINT clientPoint{};
    if (!WintabMapearParaCliente(hwnd,packet,&clientPoint)) return;
    POINT screenPoint=clientPoint;
    if (!ClientToScreen(hwnd,&screenPoint)) return;
    g_wintabLastClientPoint=clientPoint;
    g_wintabHasLastClientPoint=true;
    const bool screenPositionChanged = !g_wintabHasLastScreenPoint ||
        screenPoint.x!=g_wintabLastScreenPoint.x || screenPoint.y!=g_wintabLastScreenPoint.y;
    g_wintabLastScreenPoint=screenPoint;
    g_wintabHasLastScreenPoint=true;

    // CXO_SYSTEM ja move o cursor nos drivers conformes. Sincroniza somente quando
    // a coordenada muda, evitando chamadas repetidas durante fila de pacotes.
    if (screenPositionChanged) {
        POINT currentCursor{};
        if (!GetCursorPos(&currentCursor) || currentCursor.x!=screenPoint.x || currentCursor.y!=screenPoint.y)
            SetCursorPos(screenPoint.x,screenPoint.y);
        HCURSOR wanted=CursorSelecionadoWintab();
        if (wanted && GetCursor()!=wanted) SetCursor(wanted);
    }

    // Hit-test so e necessario para descobrir o alvo de um novo toque ou mover o
    // canvas com Espaco. Durante um traco, o alvo fica bloqueado como no Krita.
    const bool needHitTest = (contact && !wasContact) ||
        (contact && !g_wintabCanvasStrokeActive && !g_wintabExternalMouseActive) ||
        (!contact && (EspacoParaMoverAtivo() || g_wintabSpacePanHoverActive));
    bool overCanvas=false;
    if (needHitTest) {
        HWND under=WindowFromPoint(screenPoint);
        RECT canvasRect{};
        GetClientRect(hwnd,&canvasRect);
        const bool pointInCanvasClient=PtInRect(&canvasRect,clientPoint)!=FALSE;
        overCanvas=pointInCanvasClient && WintabJanelaEhFilhaDe(under,hwnd);
    }

    if (!contact) {
        if (wasContact) {
            if (g_wintabCanvasStrokeActive) {
                ProcessarPontoCaneta(hwnd,clientPoint,0.0f,false);
                canetaPointerEmContato=false;
            } else {
                WintabSoltarCliqueForaCanvas(screenPoint);
            }
            g_wintabCanvasStrokeActive=false;
            g_wintabExternalMouseActive=false;
            g_wintabMouseInjectedDown=false;
            g_wintabFallbackMouseTarget=NULL;
        }
        g_wintabContactActive=false;

        if (overCanvas && !canetaPointerEmContato && EspacoParaMoverAtivo()) {
            if (!g_wintabSpacePanHoverActive) {
                g_wintabSpacePanHoverActive=true;
                g_wintabSpacePanLastPoint=clientPoint;
                ultimoMouse=clientPoint; navegandoCanvas=true;
                SetCapture(hwnd); SetFocus(hwnd);
            } else {
                panX+=clientPoint.x-g_wintabSpacePanLastPoint.x;
                panY+=clientPoint.y-g_wintabSpacePanLastPoint.y;
                g_wintabSpacePanLastPoint=clientPoint;
                ultimoMouse=clientPoint; InvalidateRect(hwnd,NULL,FALSE);
            }
        } else if (g_wintabSpacePanHoverActive) {
            g_wintabSpacePanHoverActive=false;
            navegandoCanvas=false;
            if (GetCapture()==hwnd) ReleaseCapture();
            InvalidateRect(hwnd,NULL,FALSE);
        }
        return;
    }

    g_wintabSpacePanHoverActive=false;
    if (!wasContact) {
        if (overCanvas) {
            g_wintabCanvasStrokeActive=true;
            g_wintabExternalMouseActive=false;
            g_wintabMouseInjectedDown=false;
            g_wintabFallbackMouseTarget=NULL;
            ProcessarPontoCaneta(hwnd,clientPoint,pressure,true);
        } else {
            g_wintabCanvasStrokeActive=false;
            canetaPointerEmContato=false;
            WintabIniciarCliqueForaCanvas(screenPoint);
        }
    } else if (g_wintabCanvasStrokeActive) {
        // Assim como o Krita, mantem o alvo original durante o arrasto, mesmo
        // quando a ponta sai dos limites visiveis do canvas.
        ProcessarPontoCaneta(hwnd,clientPoint,pressure,true);
    } else if (g_wintabExternalMouseActive) {
        WintabAtualizarCliqueForaCanvas(screenPoint);
    } else if (overCanvas) {
        g_wintabCanvasStrokeActive=true;
        ProcessarPontoCaneta(hwnd,clientPoint,pressure,true);
    } else {
        WintabIniciarCliqueForaCanvas(screenPoint);
    }
    g_wintabContactActive=true;
}

static int WintabProcessarFilaPendente(HWND hwnd) {
    if (!g_wintabContext || !g_wtPacketsGet || !hwnd) return 0;
    KOD_WT_PACKET packets[64]{};
    int processed=0;
    for (int batch=0; batch<16; ++batch) {
        INT got=g_wtPacketsGet(g_wintabContext,64,packets);
        if (got<=0) break;
        if (got>64) got=64;
        for (INT i=0;i<got;++i) ProcessarPacoteWintab(hwnd,packets[i]);
        processed+=got;
        if (got<64) break;
    }
    return processed;
}

static void KodaVerificarBordaBarraTarefasUmaVez() {
    // Chamado uma única vez ao abrir cada canvas, nunca durante WM_MOUSEMOVE.
    g_canvasAutoHideTaskbarEdge=-1;
    g_canvasTaskbarInsetChecked=true;
    APPBARDATA data{}; data.cbSize=sizeof(data);
    const UINT state=(UINT)SHAppBarMessage(ABM_GETSTATE,&data);
    if((state & ABS_AUTOHIDE)==0) return;
    APPBARDATA pos{}; pos.cbSize=sizeof(pos);
    if(SHAppBarMessage(ABM_GETTASKBARPOS,&pos) &&
       (pos.uEdge==ABE_LEFT || pos.uEdge==ABE_RIGHT))
        g_canvasAutoHideTaskbarEdge=(int)pos.uEdge;
}

LRESULT CALLBACK WndProcCanvas(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(msg==WM_GETMINMAXINFO){ MINMAXINFO* mi=(MINMAXINFO*)lParam; if(mi){MONITORINFO mon{};mon.cbSize=sizeof(mon);HMONITOR hm=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);if(GetMonitorInfoW(hm,&mon)){mi->ptMaxPosition.x=mon.rcWork.left-mon.rcMonitor.left;mi->ptMaxPosition.y=mon.rcWork.top-mon.rcMonitor.top;mi->ptMaxSize.x=mon.rcWork.right-mon.rcWork.left;mi->ptMaxSize.y=mon.rcWork.bottom-mon.rcWork.top; const int inset=(g_canvasTaskbarInsetChecked && (g_canvasAutoHideTaskbarEdge==ABE_LEFT || g_canvasAutoHideTaskbarEdge==ABE_RIGHT))?2:0; if(g_canvasAutoHideTaskbarEdge==ABE_LEFT) mi->ptMaxPosition.x+=inset; if(inset) mi->ptMaxSize.x=std::max<LONG>(1,mi->ptMaxSize.x-inset);}}return 0; }
    if(msg==WM_SYSCOMMAND) {
        const UINT cmd=(UINT)(wParam & 0xFFF0u);
        // Impede restaurar/desmaximizar pelo duplo clique ou menu de sistema.
        if(cmd==SC_MAXIMIZE) return 0;
        if(cmd==SC_RESTORE && !IsIconic(hwnd)) return 0;
    }
    if (msg == WM_MOUSEMOVE) ReativarTimerFocoAutomatico();
    if (!eventoCanetaSintetico && MouseMessagePromovidaDeCaneta() &&
        (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEMOVE)) {
        return 0; // the same pen is already handled by WT_PACKET
    }
    // Wintab cursor-change: limpa o estado de botões para que um cursor/borracha
    // recém-detectado não herde um contato anterior.
    if (pressureApi == 0 && msg == KOD_WT_CSRCHANGE_MSG) {
        g_wintabLastButtons=0; g_wintabButtonsInitialized=false;
        return 0;
    }
    // Wintab: usa pacotes absolutos e escala coordenadas como a implementação do Krita.
    if (pressureApi == 0 && msg == KOD_WT_PROXIMITY_MSG) {
        // LOWORD(lParam): nonzero = entering context, zero = leaving context.
        // HIWORD reports a hardware-proximity transition and is not the same flag.
        const bool entrouNoContexto = (LOWORD(lParam) != 0);
        if (!entrouNoContexto && canetaPointerEmContato) {
            POINT pt = g_wintabHasLastClientPoint ? g_wintabLastClientPoint : POINT{0,0};
            ProcessarPontoCaneta(hwnd, pt, 0.0f, false);
            canetaPointerEmContato = false;
            pressaoAtualCaneta = 0.0f;
            desenhando = false;
            if (GetCapture() == hwnd) ReleaseCapture();
        }
        if(!entrouNoContexto && g_wintabSpacePanHoverActive) {
            g_wintabSpacePanHoverActive=false;
            navegandoCanvas=false;
            if(GetCapture()==hwnd) ReleaseCapture();
            InvalidateRect(hwnd,NULL,FALSE);
        }
        // Flush only when leaving the context. Flushing on entry discarded the
        // first real packets and could turn a stroke into a jump/straight segment.
        if (!entrouNoContexto && g_wintabContext && g_wtPacketsGet) {
            // WTPacketsGet precisa de um buffer válido; nunca passe NULL com capacidade > 0.
            KOD_WT_PACKET discard[64]{};
            for(int batch=0; batch<16; ++batch) {
                INT got=g_wtPacketsGet(g_wintabContext,64,discard);
                if(got<=0 || got<64) break;
            }
        }
        if (!entrouNoContexto) {
            WintabForcarSolturaMouseGlobal();
            g_wintabHasLastClientPoint = false;
            g_wintabHasLastScreenPoint = false;
            g_wintabLastButtons=0; g_wintabButtonsInitialized=false;
        }
        return 0;
    }
    if(pressureApi==0 && msg==KOD_WT_PACKET_MSG) {
        if(!g_wintabContext) InicializarWintab(hwnd);
        if(g_wintabContext && (g_wtPacketsGet || g_wtPacket) && (lParam == 0 || (HANDLE)lParam == g_wintabContext)) {
            KOD_WT_PACKET packets[64]{};
            bool processedAny=false;
            if(g_wtPacketsGet) {
                // Drena a fila em ordem cronológica e processa cada amostra, inclusive pressão.
                for(int batch=0;batch<16;++batch) {
                    INT got=g_wtPacketsGet(g_wintabContext,64,packets);
                    if(got<=0) break;
                    if(got>64) got=64;
                    for(INT i=0;i<got;++i) ProcessarPacoteWintab(hwnd,packets[i]);
                    processedAny=true;
                    if(got<64) break;
                }
            }
            // Wintab: wParam é o serial e lParam o HCTX. Alguns drivers expõem
            // WTPacketsGet mas não deixam o pacote da mensagem atual na fila;
            // nesse caso, usa WTPacket como fallback para não perder o traço.
            // Quando a fila foi drenada, não repete a amostra e evita segmentos duplicados.
            if(!processedAny && g_wtPacket) {
                KOD_WT_PACKET packet{};
                if(g_wtPacket(g_wintabContext,(UINT)wParam,&packet))
                    ProcessarPacoteWintab(hwnd,packet);
            }
            return 0;
        }
    }
    // Inicializa Wintab quando a janela do canvas nasce (não depende do primeiro traço).
    if(msg==WM_CREATE && pressureApi==0) InicializarWintab(hwnd);
    if(msg==WM_DESTROY && g_wintabContext) FecharWintab();
    // Windows Pointer/Ink (Windows 8+) is loaded dynamically, preserving XP build compatibility.
    if (msg == 0x0246 || msg == 0x0245 || msg == 0x0247) { // WM_POINTERDOWN/UPDATE/UP
        InicializarEntradaCanetaWindows();
        if (pressureApi == 1 && g_getPointerPenInfo) {
            const UINT32 pointerId = (UINT32)(wParam & 0xFFFF);
            if (msg == 0x0245 && g_getPointerPenInfoHistory) {
                KODANIMA_POINTER_PEN_INFO hist[128]{};
                UINT32 count = 128;
                if (g_getPointerPenInfoHistory(pointerId, &count, hist)) {
                    const UINT32 used = std::min<UINT32>(count, 128);
                    // Microsoft documents history in reverse chronological order.
                    for (INT i = (INT)used - 1; i >= 0; --i) {
                        const KODANIMA_POINTER_PEN_INFO& sample = hist[i];
                        if (sample.pointerInfo.pointerType != 3) continue; // PT_PEN
                        POINT pt = sample.pointerInfo.ptPixelLocation;
                        ScreenToClient(hwnd, &pt);
                        const UINT32 flags = sample.pointerInfo.pointerFlags;
                        const bool up = (flags & 0x00040000u) != 0;          // POINTER_FLAG_UP
                        const bool down = (flags & 0x00010000u) != 0;        // POINTER_FLAG_DOWN
                        const bool inContact = !up && (down || (flags & 0x00000004u) != 0); // INCONTACT
                        float p = (sample.penMask & 0x00000001u) ? (float)sample.pressure / 1024.0f : (inContact ? 1.0f : 0.0f);
                        p=AjustarPressaoPelosNiveis(p,1025); // Windows Pointer normaliza pressão para 0..1024
                        ProcessarPontoCaneta(hwnd, pt, p, inContact);
                    }
                    return 0;
                }
            }
            KODANIMA_POINTER_PEN_INFO pi{};
            if (g_getPointerPenInfo(pointerId, &pi) && pi.pointerInfo.pointerType == 3) { // PT_PEN
                POINT pt = pi.pointerInfo.ptPixelLocation;
                ScreenToClient(hwnd, &pt);
                const UINT32 flags = pi.pointerInfo.pointerFlags;
                const bool up = (msg == 0x0247) || ((flags & 0x00040000u) != 0);
                const bool down = (msg == 0x0246) || ((flags & 0x00010000u) != 0);
                const bool inContact = !up && (down || (flags & 0x00000004u) != 0);
                float p = (pi.penMask & 0x00000001u) ? (float)pi.pressure / 1024.0f : (inContact ? 1.0f : 0.0f);
                p=AjustarPressaoPelosNiveis(p,1025); // limite divulgado por POINTER_PEN_INFO
                ProcessarPontoCaneta(hwnd, pt, p, inContact);
                return 0;
            }
        }
    }
    if (msg == WM_CANCELMODE || msg == WM_KILLFOCUS || msg == WM_CAPTURECHANGED) {
        if (desenhando || canetaPointerEmContato || g_wintabSpacePanHoverActive) {
            bool wasSynthetic=eventoCanetaSintetico; eventoCanetaSintetico=true;
            if (desenhando) WndProcCanvas(hwnd,WM_LBUTTONUP,0,0);
            eventoCanetaSintetico=wasSynthetic;
            desenhando=false; canetaPointerEmContato=false; pressaoAtualCaneta=0.0f;
            g_wintabSpacePanHoverActive=false; navegandoCanvas=false;
            if (GetCapture()==hwnd) ReleaseCapture();
        }
    }
    switch (msg) {
        case WM_SETCURSOR:
            // O Windows pode reaplicar o cursor da classe ao mover a janela;
            // reafirma a mira selecionada pelo usuário, inclusive cursor animado .ani.
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(cursorPersonalizado ? cursorPersonalizado
                                              : LoadCursorW(NULL, MAKEINTRESOURCEW(32515)));
                return TRUE;
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int width = std::max<LONG>(1, rc.right - rc.left);
            const int height = std::max<LONG>(1, rc.bottom - rc.top);
            if (GarantirBackbufferCanvas(hdc, width, height)) {
                // O DC persistente preserva o conteúdo entre eventos. O clip limita
                // o trabalho à região invalidada (ex.: área pequena do traço).
                IntersectClipRect(g_canvasBufferDC, ps.rcPaint.left, ps.rcPaint.top,
                                  ps.rcPaint.right, ps.rcPaint.bottom);
                HBRUSH fundo = CreateSolidBrush(CorTemaFundo());
                FillRect(g_canvasBufferDC, &ps.rcPaint, fundo);
                DeleteObject(fundo);
                if (frameAtual >= 0 && frameAtual < (int)listaFrames.size()) {
                    bool composicaoTemporaria = previewAtivo || desenhando;
                    HBITMAP atual = previewAtivo ? ComporQuadroNoTempo(previewTimeSeconds)
                                   : (desenhando ? ComporQuadroComCamadas(frameAtual) : ObterQuadroCompostoCache(frameAtual));
                    if (!atual && !previewAtivo && !desenhando) {
                        // Se o cache temporário falhar por pressão de memória/GDI,
                        // renderiza diretamente em vez de apagar o conteúdo da tela.
                        atual = ComporQuadroComCamadas(frameAtual);
                        composicaoTemporaria = (atual != NULL);
                    }
                    if (!previewAtivo && onionSkinAtivo && atual && !desenhando) {
                        GarantirBitmapsCamadas(listaFrames[frameAtual]);
                        bool onionDaCamada = activeTimelineLayer>=0 && activeTimelineLayer<(int)timelineLayers.size() && timelineLayers[activeTimelineLayer].onionEnabled;
                        HBITMAP anterior = (onionDaCamada && frameAtual > 0 && activeTimelineLayer<(int)listaFrames[frameAtual-1].layerBitmaps.size()) ? listaFrames[frameAtual-1].layerBitmaps[activeTimelineLayer] : NULL;
                        HBITMAP proximo = (onionDaCamada && frameAtual + 1 < (int)listaFrames.size() && activeTimelineLayer<(int)listaFrames[frameAtual+1].layerBitmaps.size()) ? listaFrames[frameAtual+1].layerBitmaps[activeTimelineLayer] : NULL;
                        HBITMAP composto = CriarBitmapComOnionAbaixo(atual, anterior, proximo);
                        if (composto) { DesenharBitmapCanvas(g_canvasBufferDC, composto, hwnd, false, RGB(0,0,0)); DeleteObject(composto); }
                        else if (atual) DesenharBitmapCanvas(g_canvasBufferDC, atual, hwnd, false, RGB(0,0,0));
                        /* Bitmaps de camadas são propriedade dos quadros; não liberar aqui. */
                    } else if (atual) DesenharBitmapCanvas(g_canvasBufferDC, atual, hwnd, false, RGB(0,0,0));
                    if (atual && composicaoTemporaria) DeleteObject(atual);
                }
                // Pré-visualização do recorte/transformação e da colagem, usando o BMP selecionado.
                if ((transformacaoSelecaoAtiva && bitmapSelecaoRaster) || (colandoSelecao && bitmapClipboardSelecao)) {
                    bool previewTransform=transformacaoSelecaoAtiva && bitmapSelecaoRaster;
                    HBITMAP srcBmp=NULL; double sx=0.0,sy=0.0,sc=1.0,an=0.0;
                    if(colandoSelecao){srcBmp=bitmapPreviewColagemVermelho?bitmapPreviewColagemVermelho:bitmapClipboardSelecao;sx=cursorColagemBmp.x;sy=cursorColagemBmp.y;sc=colagemEscala;an=colagemAngulo*3.14159265358979323846/180.0;}
                    else if(previewTransform && GarantirCacheRotacaoSelecao()){srcBmp=bitmapCacheRotacaoSelecao;sx=bitmapCacheRotacaoX;sy=bitmapCacheRotacaoY;sc=1.0;an=0.0;}
                    else {srcBmp=bitmapSelecaoRaster;sx=centroSelecaoBmp.x-(srcBmp?((double)selecaoBmpW/2.0):0.0);sy=centroSelecaoBmp.y-(srcBmp?((double)selecaoBmpH/2.0):0.0);sc=escalaSelecaoRaster;an=anguloSelecaoRaster*3.14159265358979323846/180.0;}
                    BITMAP sb{};
                    if(srcBmp && GetObjectW(srcBmp,sizeof(sb),&sb)) {
                        // Para o cache de rotação, a transformação do objeto já foi
                        // rasterizada em memória; a prévia só transforma a vista.
                        if(previewTransform && srcBmp==bitmapCacheRotacaoSelecao){sx=bitmapCacheRotacaoX;sy=bitmapCacheRotacaoY;sc=1.0;an=0.0;}
                        double ww=sb.bmWidth*sc,hh=sb.bmHeight*sc,cx=sx+sb.bmWidth/2.0,cy=sy+sb.bmHeight/2.0;
                        const double vx[4]={-ww/2.0,ww/2.0,ww/2.0,-ww/2.0};
                        const double vy[4]={-hh/2.0,-hh/2.0,hh/2.0,hh/2.0};
                        POINT bp[4]; HBITMAP base=BitmapCamadaAtual();
                        for(int qi=0;qi<4;++qi){double xx=cx+vx[qi]*cos(an)-vy[qi]*sin(an), yy=cy+vx[qi]*sin(an)+vy[qi]*cos(an);bp[qi]=BitmapToScreenPoint(hwnd,base,xx,yy);}
                        int rawMinX=INT_MAX,rawMinY=INT_MAX,rawMaxX=INT_MIN,rawMaxY=INT_MIN;
                        for(int qi=0;qi<4;++qi){rawMinX=std::min(rawMinX,(int)bp[qi].x);rawMaxX=std::max(rawMaxX,(int)bp[qi].x);rawMinY=std::min(rawMinY,(int)bp[qi].y);rawMaxY=std::max(rawMaxY,(int)bp[qi].y);}
                        // Clip à janela antes de alocar temporários evita um bloqueio
                        // de RAM se uma seleção enorme estiver quase fora da tela.
                        RECT view{};GetClientRect(hwnd,&view);
                        int minx=std::max(0,rawMinX),miny=std::max(0,rawMinY),maxx=std::min((int)view.right,rawMaxX+1),maxy=std::min((int)view.bottom,rawMaxY+1);
                        int tw=std::max(0,maxx-minx),th=std::max(0,maxy-miny);
                        if(tw>0 && th>0 && (size_t)tw*(size_t)th<=(size_t)std::max(1,width)*(size_t)std::max(1,height)*4u){
                            HBITMAP tmp=CreateCompatibleBitmap(g_canvasBufferDC,tw,th);HDC td=tmp?CreateCompatibleDC(g_canvasBufferDC):NULL;
                            if(tmp && td){HBITMAP ot=(HBITMAP)SelectObject(td,tmp);HBRUSH bg=CreateSolidBrush(configAtual.corFundo);RECT tr={0,0,tw,th};FillRect(td,&tr,bg);DeleteObject(bg);
                                POINT tq[3]={{bp[0].x-minx,bp[0].y-miny},{bp[1].x-minx,bp[1].y-miny},{bp[3].x-minx,bp[3].y-miny}};
                                HDC src=CreateCompatibleDC(g_canvasBufferDC);HBITMAP old=(HBITMAP)SelectObject(src,srcBmp);PlgBlt(td,tq,src,0,0,sb.bmWidth,sb.bmHeight,NULL,0,0);
                                TransparentBltCompat(g_canvasBufferDC,minx,miny,tw,th,td,0,0,tw,th,configAtual.corFundo);
                                SelectObject(src,old);DeleteDC(src);SelectObject(td,ot);DeleteDC(td);DeleteObject(tmp);
                            } else {if(td)DeleteDC(td);if(tmp)DeleteObject(tmp);}
                        }
                    }
                }
                // Sobreposição de seleção e barra flutuante (não é gravada no desenho).
                if (selecaoAtiva || criandoSelecao) {
                    HPEN sp=CreatePen(PS_DOT,1,corSelecao); HGDIOBJ op=SelectObject(g_canvasBufferDC,sp); HGDIOBJ ob=SelectObject(g_canvasBufferDC,GetStockObject(HOLLOW_BRUSH));
                    if(modoLaco==1 && pontosLacoTela.size()>1){
                        MoveToEx(g_canvasBufferDC,pontosLacoTela[0].x,pontosLacoTela[0].y,NULL);
                        for(size_t pi=1;pi<pontosLacoTela.size();++pi) LineTo(g_canvasBufferDC,pontosLacoTela[pi].x,pontosLacoTela[pi].y);
                        if(selecaoAtiva) LineTo(g_canvasBufferDC,pontosLacoTela[0].x,pontosLacoTela[0].y);
                    } else Rectangle(g_canvasBufferDC,rectSelecaoTela.left,rectSelecaoTela.top,rectSelecaoTela.right,rectSelecaoTela.bottom);
                    SelectObject(g_canvasBufferDC,ob); SelectObject(g_canvasBufferDC,op); DeleteObject(sp);
                    if(selecaoAtiva){
                        RECT bar={rectSelecaoTela.left,rectSelecaoTela.bottom+6,rectSelecaoTela.left+186,rectSelecaoTela.bottom+32};
                        HBRUSH bb=CreateSolidBrush(RGB(35,39,47)); FillRect(g_canvasBufferDC,&bar,bb); DeleteObject(bb);
                        SetBkMode(g_canvasBufferDC,TRANSPARENT); SetTextColor(g_canvasBufferDC,RGB(255,255,255));
                        RECT b1={bar.left,bar.top,bar.left+34,bar.bottom},b2={bar.left+38,bar.top,bar.left+72,bar.bottom},b3={bar.left+76,bar.top,bar.left+110,bar.bottom},b4={bar.left+114,bar.top,bar.left+148,bar.bottom},b5={bar.left+152,bar.top,bar.right,bar.bottom};
                        DrawTextW(g_canvasBufferDC,L"↔",-1,&b1,DT_CENTER|DT_VCENTER|DT_SINGLELINE); DrawTextW(g_canvasBufferDC,L"C",-1,&b2,DT_CENTER|DT_VCENTER|DT_SINGLELINE); DrawTextW(g_canvasBufferDC,L"✂",-1,&b3,DT_CENTER|DT_VCENTER|DT_SINGLELINE); DrawTextW(g_canvasBufferDC,L"R",-1,&b4,DT_CENTER|DT_VCENTER|DT_SINGLELINE); DrawTextW(g_canvasBufferDC,L"S",-1,&b5,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                    }
                }
                SelectClipRgn(g_canvasBufferDC, NULL);
                int cw = ps.rcPaint.right - ps.rcPaint.left;
                int ch = ps.rcPaint.bottom - ps.rcPaint.top;
                if (cw > 0 && ch > 0) {
                    bool okRender=true;
                    if(rendererPreferido==1) okRender=ApresentarCanvasDirectX9(hwnd,g_canvasBufferBitmap,width,height);
                    else if(rendererPreferido==2) okRender=ApresentarCanvasDirectX11(hwnd,width,height);
                    else if(rendererPreferido==3) okRender=ApresentarCanvasDirectX12(hwnd,width,height);
                    else BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,cw,ch,g_canvasBufferDC,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
                    if(!okRender && rendererPreferido!=0){
                        if(rendererPreferido==1&&!g_dx9FalhaAvisada){g_dx9FalhaAvisada=true;PostMessageW(hwnd,WM_APP+91,1,0);}
                        else if(rendererPreferido==2&&!g_dx11FalhaAvisada){g_dx11FalhaAvisada=true;PostMessageW(hwnd,WM_APP+91,2,0);}
                        else if(rendererPreferido==3&&!g_dx12FalhaAvisada){g_dx12FalhaAvisada=true;PostMessageW(hwnd,WM_APP+91,3,0);}
                    }
                }
            } else {
                FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
            }
            // No modo otimizado, mostra uma linha-guia fina enquanto o usuário desenha;
            // os carimbos do pincel são aplicados ao soltar a caneta/mouse.
            if(desenhando && g_brushOptimizedStroke && !g_brushPendingSegments.empty() && frameAtual>=0 && frameAtual<(int)listaFrames.size()) {
                HBITMAP guideBmp=listaFrames[frameAtual].hBmpQuadro; BITMAP gbm{};
                if(guideBmp && GetObjectW(guideBmp,sizeof(gbm),&gbm)) {
                    HPEN guidePen=CreatePen(PS_SOLID,4,RGB(178,70,255)); HGDIOBJ oldGuide=SelectObject(hdc,guidePen);
                    HBRUSH guideBrush=CreateSolidBrush(RGB(178,70,255)); HGDIOBJ oldGuideBrush=SelectObject(hdc,guideBrush);
                    const double ga=(double)anguloCanvas*3.14159265358979323846/180.0, gca=std::cos(ga),gsa=std::sin(ga);
                    const double gcx=rc.right/2.0+panX,gcy=rc.bottom/2.0+panY;
                    POINT previous{}; bool havePrevious=false;
                    for(size_t gi=0;gi<g_brushPendingSegments.size();++gi){
                        const BrushStrokeSegment& sg=g_brushPendingSegments[gi];
                        double ax=(sg.x1-gbm.bmWidth/2.0)*zoomFactor,ay=(sg.y1-gbm.bmHeight/2.0)*zoomFactor;
                        double bx=(sg.x2-gbm.bmWidth/2.0)*zoomFactor,by=(sg.y2-gbm.bmHeight/2.0)*zoomFactor;
                        POINT pa={(LONG)std::lround(gcx+ax*gca-ay*gsa),(LONG)std::lround(gcy+ax*gsa+ay*gca)};
                        POINT pb={(LONG)std::lround(gcx+bx*gca-by*gsa),(LONG)std::lround(gcy+bx*gsa+by*gca)};
                        // Une segmentos consecutivos numa única linha-guia contínua;
                        // círculos pequenos eliminam lacunas entre amostras da caneta.
                        if(havePrevious && std::abs(previous.x-pa.x)<=3 && std::abs(previous.y-pa.y)<=3) { MoveToEx(hdc,previous.x,previous.y,NULL); LineTo(hdc,pb.x,pb.y); }
                        else { MoveToEx(hdc,pa.x,pa.y,NULL); LineTo(hdc,pb.x,pb.y); }
                        Ellipse(hdc,pa.x-2,pa.y-2,pa.x+3,pa.y+3); Ellipse(hdc,pb.x-2,pb.y-2,pb.x+3,pb.y+3);
                        previous=pb; havePrevious=true;
                    }
                    SelectObject(hdc,oldGuideBrush); SelectObject(hdc,oldGuide); DeleteObject(guideBrush); DeleteObject(guidePen);
                }
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_APP + 91: {
            int motor=(int)wParam; const wchar_t* nome=motor==1?L"DirectX 9":motor==2?L"DirectX 11":L"DirectX 12";
            wchar_t msgFalha[300]; swprintf(msgFalha,300,L"Não foi possível inicializar %s. O programa não mudou silenciosamente para GDI. Abra Preferências e selecione GDI nativo ou verifique a compatibilidade do driver.",nome);
            MessageBoxW(hwnd,msgFalha,L"Kodanima - Motor gráfico",MB_OK|MB_ICONWARNING); return 0; }
        case WM_SIZE:
            if(wParam==SIZE_MINIMIZED){ g_restoreToolsAfterMin=hwndTools&&IsWindowVisible(hwndTools);g_restoreTimelineAfterMin=hwndTimeline&&IsWindowVisible(hwndTimeline);g_restoreBrushManagerAfterMin=hwndBrushManager&&IsWindowVisible(hwndBrushManager);g_restoreToolSettingsAfterMin=hwndToolSettings&&IsWindowVisible(hwndToolSettings);g_restoreBrushPropertiesAfterMin=hwndBrushProperties&&IsWindowVisible(hwndBrushProperties);g_restoreShortcutsAfterMin=hwndShortcutPanel&&IsWindowVisible(hwndShortcutPanel);if(hwndTools)ShowWindow(hwndTools,SW_HIDE);if(hwndTimeline)ShowWindow(hwndTimeline,SW_HIDE);if(hwndBrushManager)ShowWindow(hwndBrushManager,SW_HIDE);if(hwndToolSettings)ShowWindow(hwndToolSettings,SW_HIDE);if(hwndBrushProperties)ShowWindow(hwndBrushProperties,SW_HIDE);if(hwndShortcutPanel)ShowWindow(hwndShortcutPanel,SW_HIDE); }
            else { if(g_restoreToolsAfterMin&&hwndTools&&IsWindow(hwndTools))ShowWindow(hwndTools,SW_SHOW);if(g_restoreTimelineAfterMin&&hwndTimeline&&IsWindow(hwndTimeline))ShowWindow(hwndTimeline,SW_SHOW);if(g_restoreBrushManagerAfterMin&&hwndBrushManager&&IsWindow(hwndBrushManager))ShowWindow(hwndBrushManager,SW_SHOW);if(g_restoreToolSettingsAfterMin&&hwndToolSettings&&IsWindow(hwndToolSettings))ShowWindow(hwndToolSettings,SW_SHOW);if(g_restoreBrushPropertiesAfterMin&&hwndBrushProperties&&IsWindow(hwndBrushProperties))ShowWindow(hwndBrushProperties,SW_SHOW);if(g_restoreShortcutsAfterMin&&hwndShortcutPanel&&IsWindow(hwndShortcutPanel))ShowWindow(hwndShortcutPanel,SW_SHOW);if(hwndTools&&IsWindowVisible(hwndTools))SetWindowPos(hwndTools,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);if(hwndTimeline&&IsWindowVisible(hwndTimeline))SetWindowPos(hwndTimeline,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); }
            DestruirBackbufferCanvas();
            DestruirDirectX9(); DestruirDirectX11(); DestruirDirectX12();
            g_dx9FalhaAvisada = false; g_dx11FalhaAvisada=false;g_dx12FalhaAvisada=false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            FecharTodasJanelas(); return 0;
        case WM_TIMER:
            if (wParam==ID_TIMER_PREVIEW && previewAtivo && !listaFrames.empty()) {
                double total=TimelineSecondsTotalExact(); if(total<0.001) total=0.001;
                double previousPreviewTime=previewTimeSeconds;
                previewTimeSeconds += 1.0/std::max(1,configAtual.fps);
                if(timelineLoopEnabled){
                    double loopA=std::max(0.0,std::min(total,timelineLoopStartSeconds));
                    double loopB=std::max(loopA+0.001,std::min(total,timelineLoopEndSeconds));
                    if(previewTimeSeconds>=loopB) previewTimeSeconds=loopA;
                    if(previewTimeSeconds<loopA) previewTimeSeconds=loopA;
                } else if(previewTimeSeconds>=total) previewTimeSeconds=std::fmod(previewTimeSeconds,total);
                timelinePlayheadSeconds=previewTimeSeconds;
                if(previewTimeSeconds+0.0001<previousPreviewTime) KodaAudioTocarDaTimeline(previewTimeSeconds);
                if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
                double acc=0.0; int idx=(int)listaFrames.size()-1;
                idx=-1; for(int fi=0;fi<(int)listaFrames.size();++fi){ if(!QuadroExisteNaCamada(listaFrames[fi],activeTimelineLayer)) continue; double d=DuracaoCamadaQuadro(listaFrames[fi],activeTimelineLayer); idx=fi; if(previewTimeSeconds<acc+d) break; acc+=d; } if(idx<0) idx=frameAtual;
                if(idx!=frameAtual) IrParaQuadro(idx); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
                SetTimer(hwndCanvas,ID_TIMER_PREVIEW,std::max(1,1000/std::max(1,configAtual.fps)),NULL);
                return 0;
            }
            break;
        case WM_LBUTTONDOWN:
            if (canetaPointerEmContato && !eventoCanetaSintetico) return 0;
            SetFocus(hwnd);
            ultimoMouse.x = LOWORD(lParam);
            ultimoMouse.y = HIWORD(lParam);
            if (EspacoParaMoverAtivo()) {
                navegandoCanvas = true;
                SetCapture(hwnd);
                return 0;
            }
            if (ferramentaAtual == 'I') {
                HBITMAP hb=BitmapCamadaAtual(); int px=0,py=0;
                if(hb && ScreenToBitmapPoint(hwnd,hb,POINT{LOWORD(lParam),HIWORD(lParam)},px,py) && px>=0 && py>=0 && px<configAtual.largura && py<configAtual.altura){
                    HDC dc=CreateCompatibleDC(NULL); HBITMAP old=(HBITMAP)SelectObject(dc,hb); corAtual=GetPixel(dc,px,py); SelectObject(dc,old);DeleteDC(dc);
                }
                ferramentaAtual='L'; ferramentaSelecionadaID=ID_TOOL_LAPIS; if(hwndTools)InvalidateRect(hwndTools,NULL,TRUE); InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if (ferramentaAtual == 'A') {
                if (selecaoAtiva) {
                    // Barra flutuante: três ações logo abaixo da seleção.
                    int bx=rectSelecaoTela.left, by=rectSelecaoTela.bottom+6;
                    RECT bt1={bx,by,bx+34,by+26}, bt2={bx+38,by,bx+72,by+26}, bt3={bx+76,by,bx+110,by+26}, bt4={bx+114,by,bx+148,by+26}, bt5={bx+152,by,bx+186,by+26};
                    POINT click={LOWORD(lParam),HIWORD(lParam)};
                    if(PtInRect(&bt1,click)){ if(bitmapSelecaoRaster){SalvarEstadoHistorico(listaFrames[frameAtual]);PrepararBackupTransformacao();LimparRetanguloOrigemSelecao();transformacaoSelecaoAtiva=true;operacaoSelecao=1;transformacaoRotacao=false;transformacaoEscalaForcada=false;transformacaoMoverSelecao=true;selecaoMoverRestoX=selecaoMoverRestoY=0.0;cursorTransformacao.x=LOWORD(lParam);cursorTransformacao.y=HIWORD(lParam);SetCapture(hwnd);} return 0; }
                    if(PtInRect(&bt2,click) || PtInRect(&bt3,click)) { CopiarSelecaoInterna(PtInRect(&bt3,click)); return 0; }
                    if(PtInRect(&bt4,click) || PtInRect(&bt5,click)) {
                        if(bitmapSelecaoRaster){ if(!transformacaoSelecaoAtiva){SalvarEstadoHistorico(listaFrames[frameAtual]);PrepararBackupTransformacao();LimparRetanguloOrigemSelecao();transformacaoSelecaoAtiva=true;operacaoSelecao=1;escalaSelecaoRaster=1.0f;anguloSelecaoRaster=0.0;} transformacaoRotacao=PtInRect(&bt4,click); transformacaoEscalaForcada=PtInRect(&bt5,click);transformacaoMoverSelecao=false; POINT p={LOWORD(lParam),HIWORD(lParam)}; cursorTransformacao=p; SetCapture(hwnd); } return 0;
                    }
                }
                criandoSelecao=true; inicioSelecaoTela.x=LOWORD(lParam); inicioSelecaoTela.y=HIWORD(lParam);
                ultimoPontoLacoTela=inicioSelecaoTela; rectSelecaoTela.left=rectSelecaoTela.right=inicioSelecaoTela.x; rectSelecaoTela.top=rectSelecaoTela.bottom=inicioSelecaoTela.y;
                pontosLacoTela.clear(); pontosLacoBitmap.clear(); pontosLacoTela.push_back(inicioSelecaoTela); selecaoAtiva=false; SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if (frameAtual >= 0 && frameAtual < (int)listaFrames.size()) {
                if (ferramentaAtual == 'B') {
                    HBITMAP hb = listaFrames[frameAtual].hBmpQuadro;
                    BITMAP bm{}; if (!hb || !GetObjectW(hb, sizeof(bm), &bm)) return 0;
                    RECT cr{}; GetClientRect(hwnd, &cr);
                    double sx = (double)LOWORD(lParam) - (cr.right / 2.0 + panX);
                    double sy = (double)HIWORD(lParam) - (cr.bottom / 2.0 + panY);
                    double a = -(double)anguloCanvas * 3.14159265358979323846 / 180.0;
                    double ca = cos(a), sa = sin(a);
                    double rx = sx * ca - sy * sa, ry = sx * sa + sy * ca;
                    int x = (int)((rx + bm.bmWidth * zoomFactor / 2.0) / zoomFactor);
                    int y = (int)((ry + bm.bmHeight * zoomFactor / 2.0) / zoomFactor);
                    if (x >= 0 && y >= 0 && x < bm.bmWidth && y < bm.bmHeight) {
                        SalvarEstadoHistorico(listaFrames[frameAtual]);
                        AplicarBaldeComTolerancia(hb, x, y);
                        InvalidarCacheComposicaoCanvas();
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    return 0;
                }
                SalvarEstadoHistorico(listaFrames[frameAtual]); g_brushPendingSegments.clear(); desenhando = true; pressaoAnteriorCaneta=pressaoAtualCaneta; SetCapture(hwnd);
            }
            return 0;
        case WM_RBUTTONDOWN:
            SetFocus(hwnd);
            if (GetAsyncKeyState('R') & 0x8000) {
                rotacionandoCanvas = true;
                ultimoMouse.x = LOWORD(lParam);
                ultimoMouse.y = HIWORD(lParam);
                SetCapture(hwnd);
            }
            return 0;
        case WM_MOUSEMOVE:
            if (canetaPointerEmContato && !eventoCanetaSintetico && !(wParam & MK_LBUTTON)) return 0;
            if (!eventoCanetaSintetico && !canetaPointerEmContato) pressaoAtualCaneta = 1.0f;
            // Space+drag pode iniciar o pan mesmo se o usuário já começou um laço.
            // O contorno é deslocado junto com o canvas para manter as coordenadas alinhadas.
            if (criandoSelecao && (wParam & MK_LBUTTON) && EspacoParaMoverAtivo() && !navegandoCanvas) {
                navegandoCanvas=true; ultimoMouse.x=LOWORD(lParam); ultimoMouse.y=HIWORD(lParam);
            }
            if (navegandoCanvas && (wParam & MK_LBUTTON)) {
                int dx=(int)LOWORD(lParam)-ultimoMouse.x, dy=(int)HIWORD(lParam)-ultimoMouse.y;
                panX += dx; panY += dy;
                if(criandoSelecao){
                    inicioSelecaoTela.x+=dx; inicioSelecaoTela.y+=dy;
                    ultimoPontoLacoTela.x+=dx; ultimoPontoLacoTela.y+=dy;
                    rectSelecaoTela.left+=dx; rectSelecaoTela.right+=dx; rectSelecaoTela.top+=dy; rectSelecaoTela.bottom+=dy;
                    for(size_t i=0;i<pontosLacoTela.size();++i){pontosLacoTela[i].x+=dx;pontosLacoTela[i].y+=dy;}
                }
                ultimoMouse.x=LOWORD(lParam); ultimoMouse.y=HIWORD(lParam);
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if (criandoSelecao && (wParam & MK_LBUTTON)) {
                POINT p={LOWORD(lParam),HIWORD(lParam)};
                if(modoLaco==1){
                    const POINT ultimoGuardado=pontosLacoTela.empty()?inicioSelecaoTela:pontosLacoTela.back();
                    const int dxp=p.x-ultimoGuardado.x, dyp=p.y-ultimoGuardado.y;
                    // Decima amostras quase coincidentes: mede contra o último
                    // vértice guardado, não contra a amostra anterior do mouse.
                    if(dxp*dxp+dyp*dyp>=2) pontosLacoTela.push_back(p);
                    LONG minx=pontosLacoTela.empty()?inicioSelecaoTela.x:pontosLacoTela[0].x;
                    LONG maxx=minx, miny=pontosLacoTela.empty()?inicioSelecaoTela.y:pontosLacoTela[0].y, maxy=miny;
                    for(size_t i=1;i<pontosLacoTela.size();++i){minx=std::min(minx,pontosLacoTela[i].x);maxx=std::max(maxx,pontosLacoTela[i].x);miny=std::min(miny,pontosLacoTela[i].y);maxy=std::max(maxy,pontosLacoTela[i].y);}
                    rectSelecaoTela.left=minx;rectSelecaoTela.right=maxx;rectSelecaoTela.top=miny;rectSelecaoTela.bottom=maxy;
                } else {
                    rectSelecaoTela.left=std::min(inicioSelecaoTela.x,p.x); rectSelecaoTela.right=std::max(inicioSelecaoTela.x,p.x);
                    rectSelecaoTela.top=std::min(inicioSelecaoTela.y,p.y); rectSelecaoTela.bottom=std::max(inicioSelecaoTela.y,p.y);
                }
                ultimoPontoLacoTela=p; InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(transformacaoSelecaoAtiva && bitmapSelecaoRaster) {
                POINT p={LOWORD(lParam),HIWORD(lParam)};
                if((GetKeyState(VK_SHIFT)&0x8000) || transformacaoEscalaForcada){
                    escalaSelecaoRaster+=(cursorTransformacao.y-p.y)*0.01f;
                    if(escalaSelecaoRaster<0.05f)escalaSelecaoRaster=0.05f;
                    if(escalaSelecaoRaster>20.0f)escalaSelecaoRaster=20.0f;
                } else if(transformacaoRotacao){
                    anguloSelecaoRaster+=(p.x-cursorTransformacao.x)*0.8;
                } else if(transformacaoMoverSelecao){
                    const double dx=(double)p.x-cursorTransformacao.x, dy=(double)p.y-cursorTransformacao.y;
                    const double a=(double)anguloCanvas*3.14159265358979323846/180.0;
                    selecaoMoverRestoX+=(dx*std::cos(a)+dy*std::sin(a))/std::max(0.1f,zoomFactor);
                    selecaoMoverRestoY+=(-dx*std::sin(a)+dy*std::cos(a))/std::max(0.1f,zoomFactor);
                    const int moveX=(int)std::trunc(selecaoMoverRestoX), moveY=(int)std::trunc(selecaoMoverRestoY);
                    selecaoMoverRestoX-=moveX;selecaoMoverRestoY-=moveY;
                    if(moveX || moveY){
                        centroSelecaoBmp.x+=moveX;centroSelecaoBmp.y+=moveY;
                        for(POINT& q:pontosLacoBitmap){q.x+=moveX;q.y+=moveY;}
                        AtualizarContornoSelecao(hwnd);
                    }
                }
                cursorTransformacao=p;InvalidateRect(hwnd,NULL,FALSE);return 0;
            }
            if(colandoSelecao && bitmapClipboardSelecao) {
                POINT p={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)}; HBITMAP hb=BitmapCamadaAtual(); int x=0,y=0;
                if(colagemModoRotacao){ colagemAngulo += (p.x-colagemPontoTransformacao.x)*0.8; colagemPontoTransformacao=p; }
                else if(colagemModoEscala){ colagemEscala += (colagemPontoTransformacao.y-p.y)*0.01f; if(colagemEscala<0.05f)colagemEscala=0.05f;if(colagemEscala>20.0f)colagemEscala=20.0f;colagemPontoTransformacao=p; }
                else if(ScreenToBitmapPoint(hwnd,hb,p,x,y)){ BITMAP bm{};GetObjectW(bitmapClipboardSelecao,sizeof(bm),&bm);cursorColagemBmp.x=x-bm.bmWidth/2;cursorColagemBmp.y=y-bm.bmHeight/2; }
                InvalidateRect(hwnd,NULL,FALSE);return 0;
            }
            if (rotacionandoCanvas && (wParam & MK_RBUTTON)) {
                anguloCanvas += ((int)LOWORD(lParam) - ultimoMouse.x) * 0.7f;
                ultimoMouse.x = LOWORD(lParam);
                ultimoMouse.y = HIWORD(lParam);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (desenhando && (wParam & MK_LBUTTON) &&
                frameAtual >= 0 && frameAtual < (int)listaFrames.size()) {
                POINT entradaMouse={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
                POINT pontoTraco=entradaMouse;
                // Filtro passa-baixa simples: valores maiores reduzem os saltos
                // entre amostras; laço e borracha permanecem sem estabilização.
                if(estabilizadorTracoAtivo && nivelEstabilizadorTraco>0 && ferramentaAtual=='L') {
                    const double dxRaw=(double)entradaMouse.x-ultimoMouse.x;
                    const double dyRaw=(double)entradaMouse.y-ultimoMouse.y;
                    const double resposta=1.0-0.65*((double)std::max(0,std::min(100,nivelEstabilizadorTraco))/100.0);
                    pontoTraco.x=(LONG)std::lround(ultimoMouse.x+dxRaw*resposta);
                    pontoTraco.y=(LONG)std::lround(ultimoMouse.y+dyRaw*resposta);
                    if(dxRaw*dxRaw+dyRaw*dyRaw<=1.0){pontoTraco=entradaMouse;}
                }
                HDC hdc = CreateCompatibleDC(NULL);
                HBITMAP hb = listaFrames[frameAtual].hBmpQuadro;
                HBITMAP old = (HBITMAP)SelectObject(hdc, hb);
                BITMAP bmp;
                GetObject(hb, sizeof(bmp), &bmp);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int dw = (int)(bmp.bmWidth * zoomFactor);
                int dh = (int)(bmp.bmHeight * zoomFactor);
                int cx = rc.right / 2 + panX, cy = rc.bottom / 2 + panY;
                double a = -(double)anguloCanvas * 3.14159265358979323846 / 180.0;
                double ca = cos(a), sa = sin(a);
                double sx = ((int)pontoTraco.x - cx) * ca - ((int)pontoTraco.y - cy) * sa;
                double sy = ((int)pontoTraco.x - cx) * sa + ((int)pontoTraco.y - cy) * ca;
                int x1 = (int)((sx + dw / 2.0) / zoomFactor);
                int y1 = (int)((sy + dh / 2.0) / zoomFactor);
                int x2 = x1, y2 = y1;
                // Converte o ponto anterior e o atual para coordenadas do bitmap.
                double px = ((int)ultimoMouse.x - cx) * ca - ((int)ultimoMouse.y - cy) * sa;
                double py = ((int)ultimoMouse.x - cx) * sa + ((int)ultimoMouse.y - cy) * ca;
                x1 = (int)((px + dw / 2.0) / zoomFactor);
                y1 = (int)((py + dh / 2.0) / zoomFactor);
                x2 = (int)((sx + dw / 2.0) / zoomFactor);
                y2 = (int)((sy + dh / 2.0) / zoomFactor);
                if (dw > 0 && dh > 0 && x1 >= 0 && y1 >= 0 && x1 < bmp.bmWidth && y1 < bmp.bmHeight &&
                    x2 >= 0 && y2 >= 0 && x2 < bmp.bmWidth && y2 < bmp.bmHeight) {
                    const bool pincelImportadoAtivo=(g_useImportedBrushes && g_activeBrushBitmap!=NULL);
                    const bool tamanhoPorPressaoEfetivo=pincelImportadoAtivo ? (brushSizeByPressure!=false) : (pressaoCanetaAtiva && brushSizeByPressure);
                    float larguraLapis;
                    if(tamanhoPorPressaoEfetivo && pincelImportadoAtivo) {
                        // Pincel bitmap usa o tamanho escolhido como referência; a pressão
                        // apenas varia a escala. Um piso de 35% evita ponta quase invisível
                        // quando o driver reporta pressão baixa/instável.
                        float p=std::max(0.0f,std::min(1.0f,(pressaoAnteriorCaneta+pressaoAtualCaneta)*0.5f));
                        float fator=0.35f+0.65f*(float)std::sqrt((double)p);
                        larguraLapis=std::max(1.0f,std::min(512.0f,std::max(1.0f,tamanhoLapis)*fator));
                    } else {
                        larguraLapis = tamanhoPorPressaoEfetivo ? EspessuraPorPressao((pressaoAnteriorCaneta + pressaoAtualCaneta) * 0.5f) : std::max(0.01f,tamanhoLapis);
                    }
                    int larguraApagador = pressaoBorrachaAtiva ? std::max(1,pressaoMinBorracha+(int)std::lround((pressaoMaxBorracha-pressaoMinBorracha)*pressaoAtualCaneta)) : tamanhoBorracha;
                    COLORREF corTraco=(ferramentaAtual=='E')?configAtual.corFundo:corAtual;
                    if(ferramentaAtual!='E' && g_useImportedBrushes && g_activeBrushBitmap && larguraLapis<=512.0f && g_brushOptimizedStroke) {
                        if(g_brushPendingSegments.size()<200000) g_brushPendingSegments.push_back(BrushStrokeSegment{x1,y1,x2,y2,larguraLapis,corTraco});
                    } else if(ferramentaAtual!='E' && g_useImportedBrushes && g_activeBrushBitmap && larguraLapis<=512.0f) {
                        DesenharLinhaComPontaBitmap(hdc,x1,y1,x2,y2,corTraco,larguraLapis);
                    } else if(ferramentaAtual!='E' && lapisAntiAlias && larguraLapis<=128.0f) {
                        DesenharLinhaAntiAliased(hdc,x1,y1,x2,y2,corTraco,larguraLapis,durezaPincel);
                    } else {
                        int larguraInteira=(int)std::max(1.0f,std::min(20000.0f,std::round(ferramentaAtual=='E'?(float)larguraApagador:larguraLapis)));
                        HPEN pen = CreatePen(PS_SOLID, larguraInteira, corTraco);
                        HPEN oldPen = (HPEN)SelectObject(hdc, pen); MoveToEx(hdc, x1, y1, NULL); LineTo(hdc, x2, y2);
                        SelectObject(hdc, oldPen); DeleteObject(pen);
                    }
                    InvalidarCacheComposicaoCanvas();
                }
                SelectObject(hdc, old);
                DeleteDC(hdc);
                POINT atualMouse=pontoTraco;
                int raio = (int)std::max(4.0f, tamanhoLapis * (ferramentaAtual == 'E' ? 3.0f : 1.0f) + 3.0f);
                RECT sujo;
                sujo.left = std::min((LONG)ultimoMouse.x, (LONG)atualMouse.x) - raio;
                sujo.top = std::min((LONG)ultimoMouse.y, (LONG)atualMouse.y) - raio;
                sujo.right = std::max((LONG)ultimoMouse.x, (LONG)atualMouse.x) + raio + 1;
                sujo.bottom = std::max((LONG)ultimoMouse.y, (LONG)atualMouse.y) + raio + 1;
                ultimoMouse = atualMouse;
                pressaoAnteriorCaneta = pressaoAtualCaneta;
                InvalidateRect(hwnd, &sujo, FALSE);
            }
            return 0;
        case WM_LBUTTONUP:
            if (canetaPointerEmContato && !eventoCanetaSintetico) return 0;
            if (criandoSelecao) {
                if(modoLaco==1){
                    POINT finalPoint={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
                    if(pontosLacoTela.empty() || pontosLacoTela.back().x!=finalPoint.x || pontosLacoTela.back().y!=finalPoint.y)
                        pontosLacoTela.push_back(finalPoint);
                }
                criandoSelecao=false;
                if(modoLaco==1 && pontosLacoTela.size()>=3){
                    LONG minx=pontosLacoTela[0].x,maxx=minx,miny=pontosLacoTela[0].y,maxy=miny;
                    for(const POINT& q:pontosLacoTela){minx=std::min(minx,q.x);maxx=std::max(maxx,q.x);miny=std::min(miny,q.y);maxy=std::max(maxy,q.y);}
                    rectSelecaoTela.left=minx;rectSelecaoTela.right=maxx;rectSelecaoTela.top=miny;rectSelecaoTela.bottom=maxy;
                }
                bool areaValida=(abs(rectSelecaoTela.right-rectSelecaoTela.left)>2 && abs(rectSelecaoTela.bottom-rectSelecaoTela.top)>2);
                if(modoLaco==1) areaValida=areaValida && pontosLacoTela.size()>=3;
                selecaoAtiva=areaValida;
                lacoAtivo=selecaoAtiva;
                if(selecaoAtiva && !CapturarSelecaoRaster(hwnd)){selecaoAtiva=false;lacoAtivo=false;LiberarBitmapSelecaoRaster();}
                navegandoCanvas=false;
                ReleaseCapture(); SetFocus(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if (desenhando && g_brushOptimizedStroke && !g_brushPendingSegments.empty() && frameAtual>=0 && frameAtual<(int)listaFrames.size()) {
                HDC flushDC=CreateCompatibleDC(NULL); HBITMAP flushBmp=listaFrames[frameAtual].hBmpQuadro; HGDIOBJ oldFlush=SelectObject(flushDC,flushBmp);
                for(size_t si=0;si<g_brushPendingSegments.size();++si){const BrushStrokeSegment& seg=g_brushPendingSegments[si];if(g_useImportedBrushes && g_activeBrushBitmap)DesenharLinhaComPontaBitmap(flushDC,seg.x1,seg.y1,seg.x2,seg.y2,seg.color,seg.width);else DesenharLinhaAntiAliased(flushDC,seg.x1,seg.y1,seg.x2,seg.y2,seg.color,seg.width,durezaPincel);}
                SelectObject(flushDC,oldFlush);DeleteDC(flushDC);g_brushPendingSegments.clear();InvalidarCacheComposicaoCanvas();
            }
            if (desenhando || navegandoCanvas) {
                desenhando = false;
                navegandoCanvas = false;
                ReleaseCapture();
                // Atualiza a referência onion uma vez ao concluir o traço.
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_RBUTTONUP:
            if (rotacionandoCanvas) {
                rotacionandoCanvas = false;
                ReleaseCapture();
            }
            return 0;
        case WM_MOUSEWHEEL: {
            // Zoom centrado no cursor: o pixel sob o mouse permanece no mesmo lugar.
            POINT cursor={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd,&cursor);
            HBITMAP hb=BitmapCamadaAtual(); int bx=0,by=0;
            bool haveAnchor=ScreenToBitmapPoint(hwnd,hb,cursor,bx,by);
            float oldZoom=zoomFactor;
            short delta=(short)HIWORD(wParam);
            if(delta>0) zoomFactor*=1.1f; else zoomFactor/=1.1f;
            if(zoomFactor<0.1f) zoomFactor=0.1f;
            if(zoomFactor>8.0f) zoomFactor=8.0f;
            if(haveAnchor && oldZoom!=zoomFactor){
                POINT projected=BitmapToScreenPoint(hwnd,hb,bx,by);
                panX+=cursor.x-projected.x; panY+=cursor.y-projected.y;
            }
            InvalidateRect(hwnd,NULL,FALSE); return 0;
        }
        case WM_KEYUP:
            if(wParam=='R' && transformacaoSelecaoAtiva) { transformacaoRotacao=false; return 0; }
            if(wParam==VK_SHIFT && transformacaoSelecaoAtiva) { return 0; }
            if(wParam==VK_SHIFT && colandoSelecao && colagemModoEscala){
                colagemModoEscala=false; colagemModoRotacao=false;
                BITMAP bm{}; if(GetObjectW(bitmapClipboardSelecao,sizeof(bm),&bm)){ POINT target=BitmapToScreenPoint(hwnd,BitmapCamadaAtual(),cursorColagemBmp.x+bm.bmWidth/2.0,cursorColagemBmp.y-10); ClientToScreen(hwnd,&target); SetCursorPos(target.x,target.y); }
                return 0;
            }
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) { if(transformacaoSelecaoAtiva||colandoSelecao){if(transformacaoSelecaoAtiva)RestaurarBackupTransformacao();if(colandoSelecao&&!g_pendingImageAssetRelativePath.empty()){for(size_t ai=0;ai<g_projectMediaAssets.size();){if(g_projectMediaAssets[ai].type==L"image"&&g_projectMediaAssets[ai].relativePath==g_pendingImageAssetRelativePath)g_projectMediaAssets.erase(g_projectMediaAssets.begin()+ai);else ++ai;}g_pendingImageAssetRelativePath.clear();SalvarProjetoCompleto();}transformacaoSelecaoAtiva=false;transformacaoRotacao=false;transformacaoEscalaForcada=false;transformacaoMoverSelecao=false;selecaoMoverRestoX=selecaoMoverRestoY=0.0;colandoSelecao=false;colagemModoRotacao=false;colagemModoEscala=false;colagemEscala=1.0f;colagemAngulo=0.0;if(bitmapPreviewColagemVermelho){DeleteObject(bitmapPreviewColagemVermelho);bitmapPreviewColagemVermelho=NULL;}operacaoSelecao=0;ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE);return 0;} if(selecaoAtiva||criandoSelecao){selecaoAtiva=false;criandoSelecao=false;lacoAtivo=false;operacaoSelecao=0;LiberarBitmapSelecaoRaster();InvalidateRect(hwnd,NULL,FALSE);return 0;} FecharTodasJanelas(); return 0; }
            if (wParam == VK_F11) { if(transformacaoSelecaoAtiva)RestaurarBackupTransformacao();transformacaoSelecaoAtiva=false; transformacaoMoverSelecao=false;selecaoMoverRestoX=selecaoMoverRestoY=0.0;selecaoAtiva=false; criandoSelecao=false; lacoAtivo=false; operacaoSelecao=0;transformacaoSelecaoAtiva=false;colandoSelecao=false;LiberarBitmapSelecaoRaster();InvalidateRect(hwnd,NULL,FALSE); return 0; }
            if ((GetKeyState(VK_CONTROL)&0x8000) && (wParam=='C' || wParam=='X') && selecaoAtiva) { CopiarSelecaoInterna(wParam=='X'); return 0; }
            if ((GetKeyState(VK_CONTROL)&0x8000) && wParam=='V' && bitmapClipboardSelecao) { if(bitmapPreviewColagemVermelho)DeleteObject(bitmapPreviewColagemVermelho);bitmapPreviewColagemVermelho=CriarPreviewVermelho(bitmapClipboardSelecao);colandoSelecao=true;colagemModoRotacao=false;colagemModoEscala=false;colagemEscala=1.0f;colagemAngulo=0.0;transformacaoSelecaoAtiva=false;POINT p{};GetCursorPos(&p);ScreenToClient(hwnd,&p);int x=0,y=0;ScreenToBitmapPoint(hwnd,BitmapCamadaAtual(),p,x,y);BITMAP cb{};GetObjectW(bitmapClipboardSelecao,sizeof(cb),&cb);cursorColagemBmp.x=x-cb.bmWidth/2;cursorColagemBmp.y=y-cb.bmHeight/2;SetCapture(hwnd);InvalidateRect(hwnd,NULL,FALSE);return 0; }
            if (wParam == 'R' && colandoSelecao) { colagemModoRotacao=true; colagemModoEscala=false; POINT p{}; GetCursorPos(&p); ScreenToClient(hwnd,&p); colagemPontoTransformacao=p; return 0; }
            if (wParam == 'R' && selecaoAtiva && bitmapSelecaoRaster) { if(!transformacaoSelecaoAtiva){SalvarEstadoHistorico(listaFrames[frameAtual]);PrepararBackupTransformacao();LimparRetanguloOrigemSelecao();transformacaoSelecaoAtiva=true;operacaoSelecao=1;escalaSelecaoRaster=1.0f;anguloSelecaoRaster=0.0;}transformacaoRotacao=true;transformacaoEscalaForcada=false;transformacaoMoverSelecao=false;POINT p{};GetCursorPos(&p);ScreenToClient(hwnd,&p);cursorTransformacao=p;SetCapture(hwnd);return 0; }
            if (wParam==VK_SHIFT && colandoSelecao) { colagemModoEscala=true; colagemModoRotacao=false; POINT p{};GetCursorPos(&p);ScreenToClient(hwnd,&p);colagemPontoTransformacao=p;return 0;}
            if ((wParam==VK_SHIFT) && selecaoAtiva && bitmapSelecaoRaster && !transformacaoSelecaoAtiva) {SalvarEstadoHistorico(listaFrames[frameAtual]);PrepararBackupTransformacao();LimparRetanguloOrigemSelecao();transformacaoSelecaoAtiva=true;operacaoSelecao=1;escalaSelecaoRaster=1.0f;anguloSelecaoRaster=0.0;transformacaoRotacao=false;transformacaoMoverSelecao=false;POINT p{};GetCursorPos(&p);ScreenToClient(hwnd,&p);cursorTransformacao=p;SetCapture(hwnd);return 0;}
            if (wParam == VK_RETURN && colandoSelecao) {ConfirmarColagemInterna(hwnd);ReleaseCapture();return 0;}
            if (wParam == VK_RETURN && transformacaoSelecaoAtiva) {ConfirmarTransformacaoSelecao(hwnd);ReleaseCapture();return 0; }
            if (wParam == VK_TAB) { AlternarCanvasMaximizado(); return 0; }
            if (wParam == VK_F1 || (wParam == 'C' && (GetKeyState(VK_SHIFT)&0x8000))) { AbrirConfiguracoesUsuario(hwnd); return 0; }
            if (wParam == VK_OEM_COMMA || wParam == VK_LEFT) { for(int i=frameAtual-1;i>=0;--i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)){IrParaQuadro(i);break;} return 0; }
            if (wParam == VK_OEM_PERIOD || wParam == VK_RIGHT) { for(int i=frameAtual+1;i<(int)listaFrames.size();++i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)){IrParaQuadro(i);break;} return 0; }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static double DurationValueToSeconds(double value) {
    if (durationUnit == 0) return value * 60.0;
    return value;
}
static double DurationSecondsToValue(double seconds) {
    if (durationUnit == 0) return seconds / 60.0;
    return seconds;
}
static bool QuadroExisteNaCamada(const FrameData& f, int layer) { return layer>=0 && (f.layerFrameExists.empty() || layer>=(int)f.layerFrameExists.size() || f.layerFrameExists[layer]); }
static double DuracaoCamadaQuadro(const FrameData& f, int layer) { return (layer>=0 && layer<(int)f.layerDurations.size()) ? std::max(0.001,f.layerDurations[layer]) : std::max(0.001,f.segundos); }
static double TimelineSecondsTotalExact() { double total=0.0; for(int layer=0;layer<(int)timelineLayers.size();++layer){ double t=0.0; for(const auto& f:listaFrames) if(QuadroExisteNaCamada(f,layer)) t+=DuracaoCamadaQuadro(f,layer); total=std::max(total,t); } return total; }
static int CriarQuadroNaCamada(int layer, int insertAfter) {
    if(layer<0 || layer>=(int)timelineLayers.size()) return -1;
    RegistrarHistoricoTimeline();
    FrameData f{}; f.id=1; f.segundos=1.0; f.unidadeDuracao=durationUnit;
    f.layerFrameExists.assign(timelineLayers.size(),false); f.layerFrameExists[layer]=true;
    f.layerDurations.assign(timelineLayers.size(),1.0);
    f.layerBitmaps.reserve(timelineLayers.size());
    for(size_t li=0;li<timelineLayers.size();++li) f.layerBitmaps.push_back(CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255)));
    f.hBmpQuadro=f.layerBitmaps[layer];
    int pos=0;
    if(insertAfter == -2) pos=(int)listaFrames.size(); // adicionar no fim da linha do tempo global, como novo último quadro da camada
    else {
        if(insertAfter >= 0 && insertAfter < (int)listaFrames.size() && !QuadroExisteNaCamada(listaFrames[insertAfter],layer)) {
            int previous=-1; for(int i=insertAfter-1;i>=0;--i) if(QuadroExisteNaCamada(listaFrames[i],layer)){previous=i;break;}
            insertAfter=previous;
        }
        pos=std::max(0,std::min((int)listaFrames.size(),insertAfter+1));
    }
    listaFrames.insert(listaFrames.begin()+pos,std::move(f));
    for(size_t i=0;i<listaFrames.size();++i) listaFrames[i].id=(int)i+1;
    activeTimelineLayer=selectedTimelineLayer=layer; frameAtual=selectedTimelineFrame=pos;
    if(hwndTimeline){ AtualizarCampoDuracao(hwndTimeline); InvalidateRect(hwndTimeline,NULL,TRUE); }
    InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
    SalvarProjetoCompleto(); return pos;
}
static int TimelineSecondsTotal() { return std::max(1,(int)std::ceil(TimelineSecondsTotalExact())); }
static std::wstring FormatarTempoTotal(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0) seconds = 0.0;
    unsigned long long ms = (unsigned long long)std::llround(seconds * 1000.0);
    unsigned long long mins = ms / 60000ULL; ms %= 60000ULL;
    unsigned long long secs = ms / 1000ULL; unsigned long long milli = ms % 1000ULL;
    wchar_t b[80];
    if (mins > 0) swprintf(b,80,L"%llu:%02llu.%03llu",mins,secs,milli);
    else swprintf(b,80,L"%llu.%03llu s",secs,milli);
    return b;
}
static int TimelineXForFrame(int frameIndex) {
    double t=0.0; for(int i=0;i<frameIndex && i<(int)listaFrames.size();++i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)) t+=DuracaoCamadaQuadro(listaFrames[i],selectedTimelineLayer);
    return TIMELINE_SPLIT_X+(int)std::lround((t-timelineScrollSeconds)*46.0);
}
static int FrameForTimelineX(int x) {
    double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0; double acc=0.0;
    for(int i=0;i<(int)listaFrames.size();++i){ if(!QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)) continue; double next=acc+DuracaoCamadaQuadro(listaFrames[i],selectedTimelineLayer); if(t>=acc && t<next) return i; acc=next; }
    for(int i=(int)listaFrames.size()-1;i>=0;--i) if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)) return i; return -1;
}
static int FrameForTimelineSecond(int second) {
    double acc=0.0; for(int i=0;i<(int)listaFrames.size();++i){ double next=acc+std::max(0.001,listaFrames[i].segundos); if(second<next) return i; acc=next; }
    return listaFrames.empty()?-1:(int)listaFrames.size()-1;
}
static void AtualizarCampoDuracao(HWND hwnd) {
    if (frameAtual<0 || frameAtual>=(int)listaFrames.size()) return;
    wchar_t buf[64];
    durationUnit = (listaFrames[frameAtual].unidadeDuracao == 0 ? 0 : 1);
    listaFrames[frameAtual].unidadeDuracao = durationUnit;
    GarantirBitmapsCamadas(listaFrames[frameAtual]);
    double v=DurationSecondsToValue(DuracaoCamadaQuadro(listaFrames[frameAtual],selectedTimelineLayer));
    swprintf(buf,64,L"%.9g",v);
    SetWindowTextW(GetDlgItem(hwnd,ID_EDIT_FRAME_SECONDS),buf);
    SetWindowTextW(GetDlgItem(hwnd,ID_DURATION_UNIT),durationUnit==0?L"m":L"s");
}

static bool ConfirmarDuracaoCampo(HWND hwnd, bool registrarHistorico) {
    if (frameAtual < 0 || frameAtual >= (int)listaFrames.size()) return false;
    wchar_t textValue[64] = L"";
    GetWindowTextW(GetDlgItem(hwnd, ID_EDIT_FRAME_SECONDS), textValue, 64);
    for (wchar_t* q=textValue; *q; ++q) if (*q==L',') *q=L'.';
    wchar_t* endPtr=nullptr;
    double value=wcstod(textValue, &endPtr);
    while (endPtr && (*endPtr==L' ' || *endPtr==L'\t' || *endPtr==L'\r' || *endPtr==L'\n')) ++endPtr;
    if (endPtr==textValue || (endPtr && *endPtr!=L'\0') || !std::isfinite(value) || value < 0.0) { AtualizarCampoDuracao(hwnd); return false; }
    // Unidade única para frações: em segundos, 0.5 significa meio segundo.
    // Não existe mais unidade de milissegundos para evitar ambiguidade.
    double sec=DurationValueToSeconds(value);
    if (!std::isfinite(sec) || sec < 0.001 || sec > 3600.0) { AtualizarCampoDuracao(hwnd); return false; }
    FrameData& f=listaFrames[frameAtual];
    GarantirBitmapsCamadas(f);
    double oldDuration=DuracaoCamadaQuadro(f,selectedTimelineLayer);
    bool changed=std::fabs(oldDuration-sec)>0.0000005 || f.unidadeDuracao!=durationUnit;
    if (changed && registrarHistorico) RegistrarHistoricoTimeline();
    if(selectedTimelineLayer>=0 && selectedTimelineLayer<(int)f.layerDurations.size()) f.layerDurations[selectedTimelineLayer]=sec;
    // Mantém o valor legado intacto: a duração efetiva vem de layerDurations por camada.
    f.unidadeDuracao=durationUnit;
    selectedTimelineFrame=frameAtual;
    SalvarProjetoCompleto();
    AtualizarCampoDuracao(hwnd);
    InvalidateRect(hwnd,NULL,FALSE);
    return true;
}

LRESULT CALLBACK SubclassCampoDuracao(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        HWND parent = GetParent(hwnd);
        if (parent) SendMessageW(parent, WM_APP+22, 0, (LPARAM)hwnd);
        return 0;
    }
    if (msg == WM_CHAR && wParam == L'\r') return 0;
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hwnd, SubclassCampoDuracao, subclassId);
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SubclassCampoFPS(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        HWND parent = GetParent(hwnd);
        if (parent) SendMessageW(parent, WM_APP+23, 0, (LPARAM)hwnd);
        return 0;
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hwnd, SubclassCampoFPS, subclassId);
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static int KodaTimelineFrameAtXOnLayer(int x, int layer) {
    if(layer<0 || layer>=(int)timelineLayers.size()) return -1;
    double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0;
    double acc=0.0;
    for(int fi=0;fi<(int)listaFrames.size();++fi) {
        if(!QuadroExisteNaCamada(listaFrames[fi],layer)) continue;
        double d=DuracaoCamadaQuadro(listaFrames[fi],layer);
        if(t>=acc && t<acc+d) return fi;
        acc+=d;
    }
    return -1;
}
static void KodaTimelineSelecionarTempo(double t) {
    t=std::max(0.0,std::min(TimelineSecondsTotalExact(),t));
    timelinePlayheadSeconds=t; previewTimeSeconds=t;
    KodaAudioScrub(t);
    double acc=0.0; int idx=listaFrames.empty()?0:(int)listaFrames.size()-1;
    for(int fi=0;fi<(int)listaFrames.size();++fi) {
        if(!QuadroExisteNaCamada(listaFrames[fi],activeTimelineLayer)) continue;
        double d=DuracaoCamadaQuadro(listaFrames[fi],activeTimelineLayer); idx=fi;
        if(t<acc+d) break; acc+=d;
    }
    if(!listaFrames.empty() && idx>=0) IrParaQuadro(idx);
    if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
}

LRESULT CALLBACK WndProcTimeline(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(msg==WM_DROPFILES) { KodaProcessarDrop(hwnd,(HDROP)wParam); return 0; }
    if(msg==WM_TIMER && wParam==ID_TIMER_AUDIO_SCRUB_STOP){ KillTimer(hwnd,ID_TIMER_AUDIO_SCRUB_STOP); KodaAudioParar(); return 0; }
    if (msg == WM_MOUSEMOVE) ReativarTimerFocoAutomatico();
    if (msg==WM_ERASEBKGND) return 1; // elimina apagamento antes do backbuffer ser apresentado
    if (msg==WM_CLOSE) { ConfirmarDuracaoCampo(hwnd, true); FecharTodasJanelas(); return 0; }
    // Entrada direta de caneta Windows Pointer: permite arrastar os marcadores
    // sem depender de o driver sintetizar eventos de mouse.
    if (msg==0x0246 || msg==0x0245 || msg==0x0247) {
        InicializarEntradaCanetaWindows();
        if (pressureApi==1 && g_getPointerPenInfo) {
            KODANIMA_POINTER_PEN_INFO pi{};
            if (g_getPointerPenInfo((UINT32)(wParam & 0xFFFF), &pi) && pi.pointerInfo.pointerType==3) {
                POINT pt=pi.pointerInfo.ptPixelLocation; ScreenToClient(hwnd,&pt);
                UINT32 flags=pi.pointerInfo.pointerFlags;
                bool down=(flags & 0x00010000u)!=0, up=(flags & 0x00040000u)!=0;
                if (down && timelineLoopEnabled && pt.y>=TIMELINE_RULER_TOP && pt.y<TIMELINE_RULER_TOP+alturaReguaTimeline && pt.x>=TIMELINE_SPLIT_X) {
                    int sx=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopStartSeconds-timelineScrollSeconds)*46.0);
                    int ex=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopEndSeconds-timelineScrollSeconds)*46.0);
                    int ds=std::abs(pt.x-sx), de=std::abs(pt.x-ex);
                    if(std::min(ds,de)<=18){ timelineLoopMarkerDragging=(ds<=de)?1:2; SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0; }
                }
                if (timelineLoopMarkerDragging && (msg==0x0245 || up)) {
                    double t=std::max(0.0,std::min(TimelineSecondsTotalExact(),timelineScrollSeconds+(pt.x-TIMELINE_SPLIT_X)/46.0));
                    if(timelineLoopMarkerDragging==1) timelineLoopStartSeconds=std::min(t,std::max(0.0,timelineLoopEndSeconds-0.05));
                    else timelineLoopEndSeconds=std::max(timelineLoopStartSeconds+0.05,t);
                    InvalidateRect(hwnd,NULL,FALSE);
                    if(up){ timelineLoopMarkerDragging=0; ReleaseCapture(); }
                    return 0;
                }
            }
        }
        // Caneta pressionada na régua: encaminha o gesto para a mesma lógica do mouse.
        if (pressureApi==1 && g_getPointerPenInfo && (msg==0x0246 || msg==0x0245 || msg==0x0247)) {
            KODANIMA_POINTER_PEN_INFO pi{};
            if(g_getPointerPenInfo((UINT32)(wParam & 0xFFFF),&pi) && pi.pointerInfo.pointerType==3) {
                POINT pt=pi.pointerInfo.ptPixelLocation; ScreenToClient(hwnd,&pt);
                UINT32 flags=pi.pointerInfo.pointerFlags;
                bool down=(flags & 0x00010000u)!=0, up=(flags & 0x00040000u)!=0;
                if(msg==0x0246 && down) {
                    // A caneta comum não tem botão direito: na área dos quadros,
                    // toque pressionado simula botão direito para duração/reordenação.
                    // Na régua superior, o toque continua sendo o arraste normal da pinça.
                    bool dentroReguaSuperior = (pt.y>=TIMELINE_RULER_TOP &&
                        pt.y<TIMELINE_RULER_TOP+alturaReguaTimeline && pt.x>=TIMELINE_SPLIT_X);
                    bool dentroDosQuadros = (pt.y>=TIMELINE_RULER_TOP+alturaReguaTimeline &&
                        pt.x>=TIMELINE_SPLIT_X && !listaFrames.empty());
                    g_timelinePenPlayheadGesture = dentroReguaSuperior || !dentroDosQuadros;
                    if(g_timelinePenPlayheadGesture)
                        SendMessageW(hwnd,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(pt.x,pt.y));
                    else
                        SendMessageW(hwnd,WM_RBUTTONDOWN,MK_RBUTTON,MAKELPARAM(pt.x,pt.y));
                    return 0;
                }
                if(msg==0x0245 && !up) {
                    SendMessageW(hwnd,WM_MOUSEMOVE,g_timelinePenPlayheadGesture?MK_LBUTTON:MK_RBUTTON,MAKELPARAM(pt.x,pt.y)); return 0;
                }
                if((msg==0x0247 || up)) {
                    SendMessageW(hwnd,g_timelinePenPlayheadGesture?WM_LBUTTONUP:WM_RBUTTONUP,0,MAKELPARAM(pt.x,pt.y)); g_timelinePenPlayheadGesture=false; return 0;
                }
            }
        }
    }
    switch (msg) {
        case WM_APP+22:
            ConfirmarDuracaoCampo(hwnd,true);
            SetFocus(hwnd);
            return 0;
        case WM_APP+23: {
            HWND edit = GetDlgItem(hwnd, ID_EDIT_FPS_TIMELINE);
            BOOL ok = FALSE; int fps = GetDlgItemInt(hwnd, ID_EDIT_FPS_TIMELINE, &ok, FALSE);
            if (ok && fps >= 1 && fps <= 240) { configAtual.fps=fps; SalvarProjetoCompleto(); InvalidateRect(hwnd,NULL,FALSE); }
            else SetWindowTextW(edit, std::to_wstring(configAtual.fps).c_str());
            SetFocus(hwnd); return 0;
        }
        case WM_TIMER:
            if(wParam==ID_TIMER_MEDIA_ASSET_PURGE) { KodaLimparAssetsExpirados(); return 0; }
            break;
        case WM_KEYDOWN:
            if(wParam==VK_DELETE && GetFocus()!=GetDlgItem(hwnd,ID_EDIT_FRAME_SECONDS)) { if(KodaExcluirAssetSelecionado(hwnd)) return 0; }
            if(wParam==VK_F6){ SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(ID_BTN_NOVO_QUADRO,0),0); return 0; }
            if(wParam==VK_F7){ SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(ID_BTN_INSERIR_MEIO,0),0); return 0; }
            if(wParam==VK_F12){ RECT wa{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0); RECT wr{}; GetWindowRect(hwnd,&wr); int w=wr.right-wr.left; int h=timelineDefaultHeight; SetWindowPos(hwnd,NULL,wa.left+(wa.right-wa.left-w)/2,wa.bottom-h,w,h,SWP_NOZORDER|SWP_NOACTIVATE); timelineLayerScroll=0; timelineDraggingResize=false; return 0; }
            if (wParam==VK_RETURN && GetFocus()==GetDlgItem(hwnd,ID_EDIT_FRAME_SECONDS)) { ConfirmarDuracaoCampo(hwnd,true); SetFocus(hwnd); return 0; }
            break;
        case WM_APP+21: AplicarEscalaBotoesTimeline(); return 0;
        case WM_MOUSEMOVE: {
            const int mouseMoveX=GET_X_LPARAM(lParam), mouseMoveY=GET_Y_LPARAM(lParam);
            if(timelineDurationDragFrame>=0 && g_timelineDurationTeleportPending) {
                if(mouseMoveX==g_timelineDurationTeleportPoint.x && mouseMoveY==g_timelineDurationTeleportPoint.y) {
                    g_timelineDurationTeleportPending=false; return 0; // ignora apenas o movimento sintético do salto
                }
                g_timelineDurationTeleportPending=false;
            }
            if(timelineDurationDragFrame>=0 && timelineDurationDragFrame<(int)listaFrames.size() &&
               ((wParam & MK_RBUTTON) || GetCapture()==hwnd)) {
                int x=mouseMoveX; double start=0.0;
                for(int fi=0;fi<timelineDurationDragFrame;++fi) if(QuadroExisteNaCamada(listaFrames[fi],selectedTimelineLayer)) start+=DuracaoCamadaQuadro(listaFrames[fi],selectedTimelineLayer);
                double end=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0;
                double nd=std::max(0.02,std::min(3600.0,end-start));
                GarantirBitmapsCamadas(listaFrames[timelineDurationDragFrame]);
                if(selectedTimelineLayer<(int)listaFrames[timelineDurationDragFrame].layerDurations.size()) listaFrames[timelineDurationDragFrame].layerDurations[selectedTimelineLayer]=nd;
                timelinePlayheadSeconds=std::max(0.0,start+nd); InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(timelineFrameReorderDragging && timelineReorderSourceFrame>=0 &&
               ((wParam & MK_RBUTTON) || GetCapture()==hwnd)) {
                int target=KodaTimelineFrameAtXOnLayer(GET_X_LPARAM(lParam),timelineReorderLayer);
                if(target>=0) { selectedTimelineFrame=target; InvalidateRect(hwnd,NULL,FALSE); }
                return 0;
            }
            if(g_audioClipDragging && (wParam & MK_LBUTTON)) {
                int mx=GET_X_LPARAM(lParam);
                double t=timelineScrollSeconds+(mx-TIMELINE_SPLIT_X)/46.0;
                g_audioStartSeconds=std::max(0.0,t-g_audioDragGrabOffset);
                for(auto& asset:g_projectMediaAssets) if(asset.type==L"audio" && asset.relativePath==g_audioRelativePath) { asset.startSeconds=g_audioStartSeconds; asset.durationSeconds=g_audioDurationSeconds; }
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(g_visualClipDragging && (wParam & MK_LBUTTON) && g_visualClipDragIndex>=0 && g_visualClipDragIndex<(int)g_projectMediaAssets.size()) {
                int mx=GET_X_LPARAM(lParam);
                double t=timelineScrollSeconds+(mx-TIMELINE_SPLIT_X)/46.0;
                g_projectMediaAssets[g_visualClipDragIndex].startSeconds=std::max(0.0,t-g_visualClipDragGrabOffset);
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(timelineLoopMarkerDragging) {
                int mx=GET_X_LPARAM(lParam);
                double t=std::max(0.0,std::min(TimelineSecondsTotalExact(),timelineScrollSeconds+(mx-TIMELINE_SPLIT_X)/46.0));
                if(timelineLoopMarkerDragging==1) timelineLoopStartSeconds=std::min(t,std::max(0.0,timelineLoopEndSeconds-0.05));
                else timelineLoopEndSeconds=std::max(timelineLoopStartSeconds+0.05,t);
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(timelinePlayheadDragging && (wParam & MK_LBUTTON)) {
                int x=GET_X_LPARAM(lParam); double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0;
                KodaTimelineSelecionarTempo(t); InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(timelineDraggingResize && (wParam & MK_RBUTTON)){ POINT cur{}; GetCursorPos(&cur); RECT wr{}; GetWindowRect(hwnd,&wr); RECT wa{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0); int h=static_cast<int>(std::max<LONG>(220L,std::min<LONG>(wa.bottom-wa.top,wa.bottom-cur.y))); int w=wr.right-wr.left; SetWindowPos(hwnd,NULL,wa.left+(wa.right-wa.left-w)/2,wa.bottom-h,w,h,SWP_NOZORDER|SWP_NOACTIVATE); return 0; } break;
        }
        case WM_NCLBUTTONDOWN:
            // A janela da timeline fica ancorada; arrastar bordas/título com o botão esquerdo não move nem redimensiona.
            if((int)wParam==HTCAPTION || (int)wParam==HTLEFT || (int)wParam==HTRIGHT || (int)wParam==HTTOP || (int)wParam==HTBOTTOM || (int)wParam==HTTOPLEFT || (int)wParam==HTTOPRIGHT || (int)wParam==HTBOTTOMLEFT || (int)wParam==HTBOTTOMRIGHT) return 0;
            break;
        case WM_NCRBUTTONDOWN: {
            if((int)wParam==HTCAPTION){ timelineDraggingResize=true; SetCapture(hwnd); return 0; }
            break;
        }
        case WM_NCRBUTTONUP: if(timelineDraggingResize){timelineDraggingResize=false; ReleaseCapture(); return 0;} break;
        case WM_RBUTTONUP:
            if(timelineDurationDragFrame>=0) { timelineDurationDragFrame=-1; g_timelineDurationTeleportPending=false; g_timelineDurationTeleportPoint=POINT{-1,-1}; ReleaseCapture(); SalvarProjetoCompleto(); AtualizarCampoDuracao(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            if(timelineFrameReorderDragging) {
                int target=KodaTimelineFrameAtXOnLayer(GET_X_LPARAM(lParam),timelineReorderLayer);
                int source=timelineReorderSourceFrame;
                if(source>=0 && target>=0 && source<(int)listaFrames.size() && target<(int)listaFrames.size() && source!=target) {
                    RegistrarHistoricoTimeline();
                    if(source<target) std::rotate(listaFrames.begin()+source,listaFrames.begin()+source+1,listaFrames.begin()+target+1);
                    else std::rotate(listaFrames.begin()+target,listaFrames.begin()+source,listaFrames.begin()+source+1);
                    for(int i=0;i<(int)listaFrames.size();++i) listaFrames[i].id=i+1;
                    frameAtual=target; selectedTimelineFrame=target; IrParaQuadro(target); SalvarProjetoCompleto();
                }
                timelineFrameReorderDragging=false; timelineReorderSourceFrame=-1; timelineReorderLayer=-1; ReleaseCapture(); InvalidateRect(hwnd,NULL,FALSE); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE); return 0;
            }
            if(timelineLoopMarkerDragging){timelineLoopMarkerDragging=0;ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE);return 0;} if(timelineDraggingResize){timelineDraggingResize=false; ReleaseCapture(); return 0;} break;
        case WM_CREATE: {
            SetTimer(hwnd,ID_TIMER_MEDIA_ASSET_PURGE,1000,NULL);
            // Barra minimalista: controles compactos, nomes curtos e espaço reservado ao resumo à direita.
            const int by=8, bh=26; int bx=8;
            auto btn=[&](const wchar_t* text, int w, int id, DWORD style=WS_VISIBLE|WS_CHILD){ HWND h=CreateWindowW(L"BUTTON",text,style,bx,by,w,bh,hwnd,(HMENU)(INT_PTR)id,hInstGlobal,NULL); bx+=w+2; return h; };
            auto gap=[&](int amount=5){ bx+=amount; };
            auto lbl=[&](const wchar_t* text, int w){ CreateWindowW(L"STATIC",text,WS_VISIBLE|WS_CHILD,bx,by+5,w,17,hwnd,NULL,hInstGlobal,NULL); bx+=w; };
            // Camadas
            btn(L"+ Camada",52,ID_LAYER_ADD); btn(L"Remover",46,ID_LAYER_DELETE); btn(L"↑",20,ID_LAYER_UP); btn(L"↓",20,ID_LAYER_DOWN);
            gap(5);
            // Aparência da timeline
            btn(timelineSegueTema?L"Tema" : L"Cor tema",42,ID_TIMELINE_FOLLOW_THEME); btn(L"Cor...",40,ID_TIMELINE_CUSTOM_COLOR);
            gap(5);
            // Onion skin e opacidade
            btn(L"Onion",42,ID_BTN_ONION,WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX); lbl(L"%",8); HWND editOpacity=CreateWindowW(L"EDIT",L"40",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,bx,by,28,bh,hwnd,(HMENU)ID_EDIT_ONION_OPACITY,hInstGlobal,NULL); bx+=30; SetWindowTextW(editOpacity,std::to_wstring(opacidadeOnion).c_str());
            gap(5);
            // Reprodução
            btn(L"Play",38,ID_BTN_PLAY_PREVIEW); btn(L"Loop",40,ID_BTN_LOOP_RANGE);
            gap(5);
            // Quadros
            btn(L"+ Quadro",52,ID_BTN_NOVO_QUADRO); btn(L"Inserir",44,ID_BTN_INSERIR_MEIO); btn(L"Excluir",42,ID_BTN_DELETE_FRAME,WS_VISIBLE|WS_CHILD|BS_OWNERDRAW);
            gap(5);
            // Importação única: o seletor abre mídias de imagem, áudio e vídeo.
            CreateWindowW(L"BUTTON",L"Importar",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|WS_TABSTOP,8,40,112,24,hwnd,(HMENU)ID_TIMELINE_IMPORT,hInstGlobal,NULL);
            HWND scrub=CreateWindowW(L"BUTTON",L"Ouvir ao mover a pinça",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|WS_TABSTOP,126,40,190,24,hwnd,(HMENU)ID_TIMELINE_AUDIO_SCRUB,hInstGlobal,NULL);
            SendMessageW(scrub,BM_SETCHECK,g_audioScrubEnabled?BST_CHECKED:BST_UNCHECKED,0);
            // Duração e FPS
            lbl(L"Dur.",30); HWND editDuration=CreateWindowW(L"EDIT",L"1.000",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,bx,by,40,bh,hwnd,(HMENU)ID_EDIT_FRAME_SECONDS,hInstGlobal,NULL); bx+=42; SetWindowSubclass(editDuration,SubclassCampoDuracao,1,0);
            btn(L"s",16,ID_DURATION_UNIT); gap(5); lbl(L"FPS",18); HWND editFPS=CreateWindowW(L"EDIT",std::to_wstring(configAtual.fps).c_str(),WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL,bx,by,28,bh,hwnd,(HMENU)ID_EDIT_FPS_TIMELINE,hInstGlobal,NULL); bx+=30; SetWindowSubclass(editFPS,SubclassCampoFPS,1,0);
            g_timelineControlBaseRects.clear(); EnumChildWindows(hwnd,CapturarGeometriaBaseTimeline,0); escalaBotoesTimelineAplicada=100; if(tamanhoBotoesTimeline!=100) PostMessageW(hwnd,WM_APP+21,0,0);
            break;
        }
        case WM_RBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
            int frameRowsTop=TIMELINE_RULER_TOP+alturaReguaTimeline;
            if(y>=frameRowsTop && x>=TIMELINE_SPLIT_X && !listaFrames.empty()) {
                int row=(y-frameRowsTop)/alturaLinhaCamada+timelineLayerScroll;
                if(row>=0 && row<(int)timelineLayers.size()) {
                    int fi=KodaTimelineFrameAtXOnLayer(x,row);
                    if(fi>=0) {
                        double secStart=0.0; for(int j=0;j<fi;++j) if(QuadroExisteNaCamada(listaFrames[j],row)) secStart+=DuracaoCamadaQuadro(listaFrames[j],row);
                        int left=TIMELINE_SPLIT_X+(int)std::lround((secStart-timelineScrollSeconds)*46.0);
                        // O marcador verde fica na borda ESQUERDA do quadro. Uma zona
                        // de toque maior torna-o utilizável com caneta/tablet; não deixe
                        // o gesto cair na operação de reordenação do quadro seguinte.
                        if(x>=left-5 && x<=left+14) {
                            timelineDurationDragFrame=fi;
                            selectedTimelineLayer=row; activeTimelineLayer=row;
                            selectedTimelineFrame=fi;
                            RegistrarHistoricoTimeline(); SetCapture(hwnd);
                            // O marcador verde ajusta a extremidade do quadro: salta para
                            // o fim atual em vez de alterar a duração no ponto clicado.
                            const double currentDuration=DuracaoCamadaQuadro(listaFrames[fi],row);
                            const double endTime=secStart+currentDuration;
                            RECT client{}; GetClientRect(hwnd,&client);
                            POINT destination{};
                            destination.x=TIMELINE_SPLIT_X+(int)std::lround((endTime-timelineScrollSeconds)*46.0);
                            destination.x=(LONG)std::max((LONG)TIMELINE_SPLIT_X,std::min((LONG)(client.right-2),destination.x));
                            destination.y=y;
                            g_timelineDurationTeleportPoint=destination;
                            g_timelineDurationTeleportPending=true;
                            ClientToScreen(hwnd,&destination); SetCursorPos(destination.x,destination.y);
                            InvalidateRect(hwnd,NULL,FALSE);
                            return 0;
                        }
                        // Arrastar o número do quadro com o botão direito reorganiza a sequência.
                        timelineReorderSourceFrame=fi; timelineReorderLayer=row; timelineFrameReorderDragging=true; selectedTimelineFrame=fi; selectedTimelineLayer=row; activeTimelineLayer=row; SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0;
                    }
                }
            }
            if(y>=TIMELINE_RULER_TOP && y<TIMELINE_RULER_TOP+alturaReguaTimeline && x>=TIMELINE_SPLIT_X && timelineLoopEnabled){
                int sx=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopStartSeconds-timelineScrollSeconds)*46.0);
                int ex=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopEndSeconds-timelineScrollSeconds)*46.0);
                int ds=std::abs(x-sx), de=std::abs(x-ex);
                if(std::min(ds,de)<=16){ timelineLoopMarkerDragging=(ds<=de)?1:2; SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            }
            if(y>=TIMELINE_RULER_TOP+alturaReguaTimeline && x<TIMELINE_SPLIT_X){
                int row=(y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/alturaLinhaCamada+timelineLayerScroll;
                if(row>=0 && row<(int)timelineLayers.size()){
                    selectedTimelineLayer=row;
                    HMENU menu=CreatePopupMenu();
                    std::wstring menuVisibility=KodaTraduzirTexto(timelineLayers[row].visible?L"Ocultar camada":L"Mostrar camada");
                    std::wstring menuRename=KodaTraduzirTexto(L"Renomear camada...");
                    std::wstring menuOnion=KodaTraduzirTexto(L"Onion skin nesta camada");
                    std::wstring menuFrame=KodaTraduzirTexto(L"Criar quadro nesta camada");
                    AppendMenuW(menu,MF_STRING,1,menuVisibility.c_str());
                    AppendMenuW(menu,MF_STRING,2,menuRename.c_str());
                    AppendMenuW(menu,MF_SEPARATOR,0,NULL);
                    AppendMenuW(menu,MF_STRING,3,menuOnion.c_str());
                    AppendMenuW(menu,MF_SEPARATOR,0,NULL);
                    AppendMenuW(menu,MF_STRING,4,menuFrame.c_str());
                    POINT pt{x,y}; ClientToScreen(hwnd,&pt);
                    int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL); DestroyMenu(menu);
                    if(cmd==1){ timelineLayers[row].visible=!timelineLayers[row].visible; InvalidarCacheComposicaoCanvas(); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE); }
                    else if(cmd==2){
                        if(hwndLayerRenameEdit) DestroyWindow(hwndLayerRenameEdit);
                        layerRenameIndex=row; RECT r{28,TIMELINE_RULER_TOP+alturaReguaTimeline+(row-timelineLayerScroll)*alturaLinhaCamada,178,TIMELINE_RULER_TOP+alturaReguaTimeline+(row-timelineLayerScroll)*alturaLinhaCamada+alturaLinhaCamada-2};
                        hwndLayerRenameEdit=CreateWindowW(L"EDIT",timelineLayers[row].name.c_str(),WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,r.left,r.top,r.right-r.left,r.bottom-r.top,hwnd,(HMENU)ID_EDIT_LAYER_RENAME,hInstGlobal,NULL);
                        SetFocus(hwndLayerRenameEdit); SendMessageW(hwndLayerRenameEdit,EM_SETSEL,0,-1);
                    } else if(cmd==3){ timelineLayers[row].onionEnabled=!timelineLayers[row].onionEnabled; }
                    else if(cmd==4){ selectedTimelineLayer=activeTimelineLayer=row; int anchor=frameAtual; int created=CriarQuadroNaCamada(row,anchor); if(created>=0) IrParaQuadro(created); }
                    InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto();
                }
            }
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            int x=LOWORD(lParam), y=HIWORD(lParam);
            if(y>=TIMELINE_RULER_TOP+alturaReguaTimeline && x<TIMELINE_SPLIT_X){ int row=(y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/alturaLinhaCamada+timelineLayerScroll; if(row>=0 && row<(int)timelineLayers.size()){ if(hwndLayerRenameEdit) DestroyWindow(hwndLayerRenameEdit); layerRenameIndex=row; hwndLayerRenameEdit=CreateWindowW(L"EDIT",timelineLayers[row].name.c_str(),WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,28,TIMELINE_RULER_TOP+alturaReguaTimeline+(row-timelineLayerScroll)*alturaLinhaCamada,150,alturaLinhaCamada-2,hwnd,(HMENU)ID_EDIT_LAYER_RENAME,hInstGlobal,NULL); SetFocus(hwndLayerRenameEdit); SendMessageW(hwndLayerRenameEdit,EM_SETSEL,0,-1); } return 0; }
            break;
        }
        case WM_MOUSEWHEEL: {
            POINT pt{GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)}; ScreenToClient(hwnd,&pt);
            if(pt.y>=TIMELINE_RULER_TOP && pt.y<TIMELINE_RULER_TOP+alturaReguaTimeline){ RECT rc{}; GetClientRect(hwnd,&rc); int visibleSec=std::max<int>(1,static_cast<int>((rc.right-TIMELINE_SPLIT_X)/46)); int maxScroll=std::max(0,(int)std::ceil(TimelineSecondsTotalExact())-visibleSec); int step=(GET_WHEEL_DELTA_WPARAM(wParam)>0)?-1:1; timelineScrollSeconds=std::max(0,std::min(maxScroll,timelineScrollSeconds+step)); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            if(pt.y>=TIMELINE_RULER_TOP+alturaReguaTimeline && pt.x<TIMELINE_SPLIT_X){ RECT rc{}; GetClientRect(hwnd,&rc); int visibleRows=std::max(1,static_cast<int>((rc.bottom-(TIMELINE_RULER_TOP+alturaReguaTimeline))/std::max(1,alturaLinhaCamada))); int maxRows=std::max(0,(int)timelineLayers.size()+2-visibleRows); int step=(GET_WHEEL_DELTA_WPARAM(wParam)>0)?-1:1; timelineLayerScroll = static_cast<int>(std::max<int>(0, std::min<int>(maxRows, static_cast<int>(timelineLayerScroll) + step))); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            break;
        }
        case WM_LBUTTONUP:
            if(g_audioClipDragging){ g_audioClipDragging=false; ReleaseCapture(); SalvarProjetoCompleto(); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            if(g_visualClipDragging){ g_visualClipDragging=false; g_visualClipDragIndex=-1; ReleaseCapture(); SalvarProjetoCompleto(); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            if(timelineLoopMarkerDragging){timelineLoopMarkerDragging=0;ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE);return 0;}
            if(timelinePlayheadDragging){ timelinePlayheadDragging=false; ReleaseCapture(); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            break;
        case WM_LBUTTONDOWN: {
            int x=GET_X_LPARAM(lParam), y=GET_Y_LPARAM(lParam);
            if(timelineLoopEnabled && y>=TIMELINE_RULER_TOP && y<TIMELINE_RULER_TOP+alturaReguaTimeline && x>=TIMELINE_SPLIT_X){
                int sx=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopStartSeconds-timelineScrollSeconds)*46.0);
                int ex=TIMELINE_SPLIT_X+(int)std::lround((timelineLoopEndSeconds-timelineScrollSeconds)*46.0);
                int ds=std::abs(x-sx), de=std::abs(x-ex);
                if(std::min(ds,de)<=16){ timelineLoopMarkerDragging=(ds<=de)?1:2; SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0; }
            }
            const int audioRowLogical=(int)timelineLayers.size();
            const int audioRowY=(TIMELINE_RULER_TOP+alturaReguaTimeline)+(audioRowLogical-timelineLayerScroll)*alturaLinhaCamada;
            if(y>=audioRowY && y<audioRowY+alturaLinhaCamada && x>=TIMELINE_SPLIT_X) {
                for(int ai=(int)g_projectMediaAssets.size()-1;ai>=0;--ai) {
                    const auto& aa=g_projectMediaAssets[ai]; if(aa.type!=L"audio") continue;
                    double dur=aa.durationSeconds>0.0?aa.durationSeconds:std::max(0.5,TimelineSecondsTotalExact()-aa.startSeconds);
                    int left=TIMELINE_SPLIT_X+(int)std::lround((aa.startSeconds-timelineScrollSeconds)*46.0);
                    int right=TIMELINE_SPLIT_X+(int)std::lround((aa.startSeconds+dur-timelineScrollSeconds)*46.0);
                    if(x>=left-4 && x<=right+4) {
                        double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0;
                        g_selectedMediaAssetIndex=ai;
                        if(aa.relativePath==g_audioRelativePath) { g_audioDragGrabOffset=std::max(0.0,t-g_audioStartSeconds); g_audioClipDragging=true; }
                        SetCapture(hwnd); return 0;
                    }
                }
            }
            const int visualLogicalRow=(int)timelineLayers.size()+1;
            const int visualRowY=(TIMELINE_RULER_TOP+alturaReguaTimeline)+(visualLogicalRow-timelineLayerScroll)*alturaLinhaCamada;
            if(y>=visualRowY && y<visualRowY+alturaLinhaCamada && x>=TIMELINE_SPLIT_X) {
                for(int vi=(int)g_projectMediaAssets.size()-1;vi>=0;--vi) {
                    const auto& asset=g_projectMediaAssets[vi];
                    if(asset.type!=L"video" && asset.type!=L"gif") continue;
                    double dur=asset.durationSeconds>0.0?asset.durationSeconds:3.0;
                    int left=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds-timelineScrollSeconds)*46.0);
                    int right=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds+dur-timelineScrollSeconds)*46.0);
                    if(x>=left-4 && x<=right+4) {
                        double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0;
                        g_visualClipDragIndex=vi; g_selectedMediaAssetIndex=vi; g_visualClipDragGrabOffset=std::max(0.0,t-asset.startSeconds);
                        g_visualClipDragging=true; SetCapture(hwnd); return 0;
                    }
                }
            }
            double currentPinTime=previewAtivo?previewTimeSeconds:timelinePlayheadSeconds;
            int currentPinX=TIMELINE_SPLIT_X+(int)std::lround((currentPinTime-timelineScrollSeconds)*46.0);
            if(y>=TIMELINE_RULER_TOP && x>=TIMELINE_SPLIT_X && (y<TIMELINE_RULER_TOP+alturaReguaTimeline || std::abs(x-currentPinX)<=8)){
                if(previewAtivo) { if(hwndCanvas) KillTimer(hwndCanvas,ID_TIMER_PREVIEW); previewAtivo=false; if(hwndTimeline) SetWindowTextW(GetDlgItem(hwndTimeline,ID_BTN_PLAY_PREVIEW),L"▶ Play"); }
                timelinePlayheadDragging=true; SetCapture(hwnd);
                double t=timelineScrollSeconds+(x-TIMELINE_SPLIT_X)/46.0; KodaTimelineSelecionarTempo(t);
                InvalidateRect(hwnd,NULL,FALSE); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); return 0;
            }
            if(y>=TIMELINE_RULER_TOP+alturaReguaTimeline && x<TIMELINE_SPLIT_X){ int row=(y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/alturaLinhaCamada+timelineLayerScroll; if(row>=0 && row<(int)timelineLayers.size()){ if(x<30){ timelineLayers[row].visible=!timelineLayers[row].visible; InvalidarCacheComposicaoCanvas(); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE); SalvarProjetoCompleto(); } else { selectedTimelineLayer=row; activeTimelineLayer=row; SincronizarAliasCamadaAtiva(); if(!listaFrames.empty() && !QuadroExisteNaCamada(listaFrames[frameAtual],row)){ int nearest=-1; for(int i=frameAtual-1;i>=0;--i) if(QuadroExisteNaCamada(listaFrames[i],row)){nearest=i;break;} if(nearest<0) for(int i=frameAtual+1;i<(int)listaFrames.size();++i) if(QuadroExisteNaCamada(listaFrames[i],row)){nearest=i;break;} if(nearest>=0) IrParaQuadro(nearest); } AtualizarCampoDuracao(hwnd); InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); } InvalidateRect(hwnd,NULL,FALSE); } return 0; }
            if(y>=TIMELINE_RULER_TOP+alturaReguaTimeline && x>=TIMELINE_SPLIT_X && !listaFrames.empty()){ int row=(y-(TIMELINE_RULER_TOP+alturaReguaTimeline))/alturaLinhaCamada+timelineLayerScroll; if(row>=0 && row<(int)timelineLayers.size()){ selectedTimelineLayer=row; activeTimelineLayer=row; SincronizarAliasCamadaAtiva(); } int col=FrameForTimelineX(x); if(col>=0){ IrParaQuadro(col); selectedTimelineFrame=col; AtualizarCampoDuracao(hwnd); InvalidateRect(hwnd,NULL,FALSE); } return 0; }
            break;
        }
        case WM_COMMAND:
            if(LOWORD(wParam)==ID_TIMELINE_IMPORT || LOWORD(wParam)==ID_TIMELINE_IMPORT_IMAGE){ KodaMostrarDialogoImportar(hwnd); return 0; }
            if(LOWORD(wParam)==ID_TIMELINE_AUDIO_SCRUB){ g_audioScrubEnabled=SendMessageW(GetDlgItem(hwnd,ID_TIMELINE_AUDIO_SCRUB),BM_GETCHECK,0,0)==BST_CHECKED; SalvarConfiguracoesUsuario(); return 0; }
            if(LOWORD(wParam)==ID_BTN_LOOP_RANGE){
                timelineLoopEnabled=!timelineLoopEnabled;
                double total=std::max(0.001,TimelineSecondsTotalExact());
                if(timelineLoopEndSeconds<=0.0 || timelineLoopEndSeconds>total) timelineLoopEndSeconds=total;
                timelineLoopStartSeconds=std::max(0.0,std::min(total,timelineLoopStartSeconds));
                timelineLoopEndSeconds=std::max(timelineLoopStartSeconds+0.05,std::min(total,timelineLoopEndSeconds));
                SetWindowTextW(GetDlgItem(hwnd,ID_BTN_LOOP_RANGE),timelineLoopEnabled?L"Loop ON":L"Loop");
                InvalidateRect(hwnd,NULL,FALSE); return 0;
            }
            if(LOWORD(wParam)==ID_TIMELINE_FOLLOW_THEME){timelineSegueTema=!timelineSegueTema;SetWindowTextW(GetDlgItem(hwnd,ID_TIMELINE_FOLLOW_THEME),timelineSegueTema?L"Timeline: tema":L"Timeline: cor");SalvarConfiguracoesUsuario();InvalidateRect(hwnd,NULL,FALSE);return 0;}
            if(LOWORD(wParam)==ID_TIMELINE_CUSTOM_COLOR){CHOOSECOLORW cc{};static COLORREF custom[16]={0};cc.lStructSize=sizeof(cc);cc.hwndOwner=hwnd;cc.lpCustColors=custom;cc.rgbResult=corTimelinePersonalizada;cc.Flags=CC_FULLOPEN|CC_RGBINIT|CC_ENABLEHOOK;cc.lpfnHook=KodaHookSeletorCor;if(ChooseColorW(&cc)){corTimelinePersonalizada=cc.rgbResult;timelineSegueTema=false;SetWindowTextW(GetDlgItem(hwnd,ID_TIMELINE_FOLLOW_THEME),L"Timeline: cor");SalvarConfiguracoesUsuario();InvalidateRect(hwnd,NULL,FALSE);}return 0;}
            if (LOWORD(wParam)==ID_EDIT_LAYER_RENAME && HIWORD(wParam)==EN_KILLFOCUS) {
                if(hwndLayerRenameEdit && layerRenameIndex>=0 && layerRenameIndex<(int)timelineLayers.size()) { wchar_t name[128]; GetWindowTextW(hwndLayerRenameEdit,name,128); if(name[0]) timelineLayers[layerRenameIndex].name=name; DestroyWindow(hwndLayerRenameEdit); hwndLayerRenameEdit=NULL; layerRenameIndex=-1; InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto(); } return 0;
            } else if (LOWORD(wParam)==ID_DURATION_UNIT) {
                if(frameAtual>=0 && frameAtual<(int)listaFrames.size()) {
                    ConfirmarDuracaoCampo(hwnd, true);
                    durationUnit=(durationUnit==0)?1:0;
                    listaFrames[frameAtual].unidadeDuracao=durationUnit;
                    AtualizarCampoDuracao(hwnd); InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto();
                } return 0;
            } else if (LOWORD(wParam) == ID_LAYER_ADD) {
                RegistrarHistoricoTimeline();
                wchar_t nome[64]; swprintf(nome,64,L"Camada %d",(int)timelineLayers.size()+1);
                timelineLayers.push_back({nome,true});
                for (auto& f : listaFrames) {
                    if(f.layerFrameExists.empty()) f.layerFrameExists.assign(timelineLayers.size()-1,true);
                    if(f.layerDurations.size()<timelineLayers.size()-1) f.layerDurations.resize(timelineLayers.size()-1,std::max(0.001,f.segundos));
                    if(f.layerBitmaps.size()<timelineLayers.size()-1) while(f.layerBitmaps.size()<timelineLayers.size()-1) f.layerBitmaps.push_back(CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255)));
                    f.layerFrameExists.push_back(false);
                    f.layerDurations.push_back(1.0);
                    f.layerBitmaps.push_back(CriarBitmapEmBranco(configAtual.largura,configAtual.altura,RGB(255,255,255)));
                }
                activeTimelineLayer=selectedTimelineLayer=(int)timelineLayers.size()-1; SincronizarAliasCamadaAtiva();
                InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto(); return 0;
            } else if (LOWORD(wParam) == ID_LAYER_DELETE) {
                if (timelineLayers.size()>1 && selectedTimelineLayer>=0 && selectedTimelineLayer<(int)timelineLayers.size()) RegistrarHistoricoTimeline();
                if (timelineLayers.size()>1 && selectedTimelineLayer>=0 && selectedTimelineLayer<(int)timelineLayers.size()) {
                    for(auto& f:listaFrames){ GarantirBitmapsCamadas(f); if(f.layerBitmaps[selectedTimelineLayer]) DeleteObject(f.layerBitmaps[selectedTimelineLayer]); f.layerBitmaps.erase(f.layerBitmaps.begin()+selectedTimelineLayer); if(selectedTimelineLayer<(int)f.layerDurations.size()) f.layerDurations.erase(f.layerDurations.begin()+selectedTimelineLayer); if(selectedTimelineLayer<(int)f.layerFrameExists.size()) f.layerFrameExists.erase(f.layerFrameExists.begin()+selectedTimelineLayer); }
                    timelineLayers.erase(timelineLayers.begin()+selectedTimelineLayer); selectedTimelineLayer=std::max(0,selectedTimelineLayer-1); activeTimelineLayer=selectedTimelineLayer; SincronizarAliasCamadaAtiva();
                    InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto();
                } return 0;
            } else if (LOWORD(wParam) == ID_LAYER_UP || LOWORD(wParam) == ID_LAYER_DOWN) {
                int a=selectedTimelineLayer, b=a+(LOWORD(wParam)==ID_LAYER_UP?-1:1); if(a>=0 && a<(int)timelineLayers.size() && b>=0 && b<(int)timelineLayers.size()) RegistrarHistoricoTimeline();
                if(a>=0 && a<(int)timelineLayers.size() && b>=0 && b<(int)timelineLayers.size()) {
                    std::swap(timelineLayers[a],timelineLayers[b]); for(auto& f:listaFrames){ GarantirBitmapsCamadas(f); std::swap(f.layerBitmaps[a],f.layerBitmaps[b]); if(a<(int)f.layerDurations.size() && b<(int)f.layerDurations.size()) std::swap(f.layerDurations[a],f.layerDurations[b]); if(a<(int)f.layerFrameExists.size() && b<(int)f.layerFrameExists.size()) { bool tempExists = f.layerFrameExists[a]; f.layerFrameExists[a] = f.layerFrameExists[b]; f.layerFrameExists[b] = tempExists; } }
                    if(activeTimelineLayer==a) activeTimelineLayer=b; else if(activeTimelineLayer==b) activeTimelineLayer=a;
                    selectedTimelineLayer=b; SincronizarAliasCamadaAtiva(); InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); InvalidateRect(hwnd,NULL,FALSE); SalvarProjetoCompleto();
                } return 0;
            } else if (LOWORD(wParam) == ID_BTN_DELETE_FRAME) {
                RegistrarHistoricoTimeline(); ExcluirQuadroAtual();
            } else if (LOWORD(wParam) == ID_BTN_INSERIR_MEIO) {
                InserirQuadroNoMeio();
            } else if (LOWORD(wParam) == ID_BTN_PLAY_PREVIEW || LOWORD(wParam) == ID_BTN_PLAY) {
                AlternarPreview();
            } else if (LOWORD(wParam) == ID_BTN_NOVO_QUADRO) {
                int created=CriarQuadroNaCamada(selectedTimelineLayer,-2);
                if(created>=0){ IrParaQuadro(created); AtualizarCampoDuracao(hwnd); InvalidateRect(hwndCanvas,NULL,FALSE); InvalidateRect(hwnd,NULL,FALSE); }
            } else if (LOWORD(wParam) == 406) {
                if (frameAtual >= 0 && frameAtual < (int)listaFrames.size()) {
                    listaFrames[frameAtual].segundos++;
                    AtualizarCampoDuracao(hwnd);
                    SalvarProjetoCompleto();
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            } else if (LOWORD(wParam) == ID_EDIT_ONION_OPACITY && HIWORD(wParam) == EN_KILLFOCUS) {
                wchar_t opText[32] = L"40";
                GetWindowTextW(GetDlgItem(hwnd, ID_EDIT_ONION_OPACITY), opText, 32);
                opacidadeOnion = std::max(0, std::min(100, _wtoi(opText)));
                SetWindowTextW(GetDlgItem(hwnd, ID_EDIT_ONION_OPACITY), std::to_wstring(opacidadeOnion).c_str());
                InvalidarCacheComposicaoCanvas(); if (hwndCanvas) InvalidateRect(hwndCanvas, NULL, FALSE);
            } else if (LOWORD(wParam) == ID_EDIT_FRAME_SECONDS && HIWORD(wParam) == EN_KILLFOCUS) {
                ConfirmarDuracaoCampo(hwnd, true);
            } else if (LOWORD(wParam) == ID_BTN_ONION) {
                onionSkinAtivo = (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                InvalidarCacheComposicaoCanvas(); if (hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
            }
            return 0;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis && dis->CtlID == ID_BTN_DELETE_FRAME) {
                HBRUSH brush = CreateSolidBrush(RGB(128, 24, 48)); // vinho
                FillRect(dis->hDC, &dis->rcItem, brush); DeleteObject(brush);
                SetBkMode(dis->hDC, TRANSPARENT); SetTextColor(dis->hDC, RGB(255,255,255));
                { std::wstring label=KodaTraduzirTexto(L"Excluir"); DrawTextW(dis->hDC,label.c_str(),-1,(LPRECT)&dis->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS); }
                if (dis->itemState & ODS_FOCUS) DrawFocusRect(dis->hDC, &dis->rcItem);
                return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC paintDC = BeginPaint(hwnd, &ps);
            RECT client{}; GetClientRect(hwnd,&client);
            int bw=std::max(1,(int)(client.right-client.left)), bh=std::max(1,(int)(client.bottom-client.top));
            HDC hdc=CreateCompatibleDC(paintDC);
            HBITMAP backBitmap=CreateCompatibleBitmap(paintDC,bw,bh);
            HGDIOBJ oldBackBitmap=SelectObject(hdc,backBitmap);
            COLORREF bgTimeline=timelineSegueTema?(temaAtual==1?RGB(238,240,244):RGB(28,30,36)):corTimelinePersonalizada;
            HBRUSH backBrush=CreateSolidBrush(bgTimeline);
            RECT fullClient{0,0,bw,bh}; FillRect(hdc,&fullClient,backBrush); DeleteObject(backBrush);
            // Faixas horizontais discretas atrás dos controles: cabeçalho de transporte e cabeçalho de camadas.
            COLORREF faixaSuperior=timelineSegueTema?(temaAtual==1?RGB(220,224,232):temaAtual==2?RGB(62,25,42):temaAtual==3?RGB(38,25,48):RGB(38,41,49)):RGB(std::max(0,GetRValue(corTimelinePersonalizada)-8),std::max(0,GetGValue(corTimelinePersonalizada)-8),std::max(0,GetBValue(corTimelinePersonalizada)-8));
            COLORREF faixaInferior=timelineSegueTema?(temaAtual==1?RGB(230,233,239):temaAtual==2?RGB(52,22,37):temaAtual==3?RGB(33,22,43):RGB(33,36,44)):RGB(std::max(0,GetRValue(corTimelinePersonalizada)-13),std::max(0,GetGValue(corTimelinePersonalizada)-13),std::max(0,GetBValue(corTimelinePersonalizada)-13));
            HBRUSH faixa1=CreateSolidBrush(faixaSuperior); RECT rcFaixa1{0,0,bw,36}; FillRect(hdc,&rcFaixa1,faixa1); DeleteObject(faixa1);
            HBRUSH faixa2=CreateSolidBrush(faixaInferior); RECT rcFaixa2{0,36,bw,71}; FillRect(hdc,&rcFaixa2,faixa2); DeleteObject(faixa2);
            HPEN sepPen=CreatePen(PS_SOLID,1,timelineSegueTema?(temaAtual==1?RGB(185,191,202):RGB(57,62,73)):RGB(std::max(0,GetRValue(corTimelinePersonalizada)-24),std::max(0,GetGValue(corTimelinePersonalizada)-24),std::max(0,GetBValue(corTimelinePersonalizada)-24)));
            HGDIOBJ oldSep=SelectObject(hdc,sepPen); MoveToEx(hdc,0,35,NULL); LineTo(hdc,bw,35); MoveToEx(hdc,0,70,NULL); LineTo(hdc,bw,70); MoveToEx(hdc,TIMELINE_SPLIT_X-1,TIMELINE_RULER_TOP,NULL); LineTo(hdc,TIMELINE_SPLIT_X-1,bh); SelectObject(hdc,oldSep); DeleteObject(sepPen);
            SetBkMode(hdc, TRANSPARENT);
            int rulerTop=TIMELINE_RULER_TOP; int layerTop=rulerTop+alturaReguaTimeline; RECT header{8,rulerTop,TIMELINE_SPLIT_X-2,layerTop-1}; COLORREF tlBase=timelineSegueTema?(temaAtual==2?RGB(112,25,58):temaAtual==1?RGB(220,224,230):temaAtual==3?RGB(45,25,62):RGB(48,51,60)):corTimelinePersonalizada; HBRUSH hb=CreateSolidBrush(tlBase); FillRect(hdc,&header,hb); DeleteObject(hb);
            SetTextColor(hdc,RGB(230,233,240)); { std::wstring head=KodaTraduzirTexto(L"CAMADAS / ORDEM"); DrawTextW(hdc,head.c_str(),-1,&header,DT_LEFT|DT_VCENTER|DT_SINGLELINE); }
            double totalDuration=std::max(TimelineSecondsTotalExact(),g_audioPath.empty()?0.0:g_audioStartSeconds+std::max(0.0,g_audioDurationSeconds));
            for(const auto& asset:g_projectMediaAssets) if(asset.type==L"video"||asset.type==L"gif") totalDuration=std::max(totalDuration,asset.startSeconds+std::max(0.5,asset.durationSeconds));
            int totalPixels=(int)std::ceil((totalDuration-timelineScrollSeconds)*46.0);
            RECT frameHeader{TIMELINE_SPLIT_X,rulerTop,std::min<LONG>(client.right,TIMELINE_SPLIT_X+std::max(0,totalPixels)),layerTop-1};
            if(frameHeader.right<frameHeader.left) frameHeader.right=frameHeader.left;
            COLORREF tlRuler=timelineSegueTema?(temaAtual==2?RGB(132,35,70):temaAtual==1?RGB(190,198,210):temaAtual==3?RGB(62,35,84):RGB(48,65,88)):corTimelinePersonalizada; HBRUSH hbh=CreateSolidBrush(tlRuler); FillRect(hdc,&frameHeader,hbh); DeleteObject(hbh);
            // Régua fixa de tempo: 10 subdivisões por segundo; marca maior a cada 0,5 s.
            // A régua termina exatamente no fim da animação, não na largura da janela.
            HPEN rulerPen=CreatePen(PS_SOLID,1,RGB(160,177,198)); HGDIOBJ oldRulerPen=SelectObject(hdc,rulerPen);
            double startTime=(double)timelineScrollSeconds;
            int firstTick=(int)std::floor(startTime*10.0);
            int lastTick=(int)std::ceil(totalDuration*10.0);
            for(int ti=firstTick;ti<=lastTick;++ti){
                double t=ti/10.0;
                if(t>totalDuration+0.00001) break;
                int x=TIMELINE_SPLIT_X+(int)std::lround((t-startTime)*46.0);
                if(x<TIMELINE_SPLIT_X || x>client.right || x>frameHeader.right) continue;

                // 10 marcações por segundo. A 5ª subdivisão (meio segundo)
                // é mais grossa; os números aparecem somente em segundos inteiros.
                bool halfSecond=(ti%5)==0;
                bool wholeSecond=(ti%10)==0;
                HPEN tickPen=CreatePen(PS_SOLID,halfSecond?2:1,RGB(160,177,198));
                HGDIOBJ oldTickPen=SelectObject(hdc,tickPen);
                MoveToEx(hdc,x,halfSecond?rulerTop:rulerTop+alturaReguaTimeline/2,NULL);
                LineTo(hdc,x,layerTop-1);
                SelectObject(hdc,oldTickPen);
                DeleteObject(tickPen);

                if(wholeSecond){
                    wchar_t label[24];
                    swprintf(label,24,L"%d",ti/10);
                    RECT tr{x-17,rulerTop,x+20,rulerTop+alturaReguaTimeline/2+2};
                    SetTextColor(hdc,RGB(240,243,248));
                    DrawTextW(hdc,label,-1,&tr,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
                }
            }
            SelectObject(hdc,oldRulerPen); DeleteObject(rulerPen);
            if(timelineLoopEnabled){
                double loopA=std::max(0.0,std::min(totalDuration,timelineLoopStartSeconds));
                double loopB=std::max(loopA,std::min(totalDuration,timelineLoopEndSeconds));
                int loopAX=TIMELINE_SPLIT_X+(int)std::lround((loopA-timelineScrollSeconds)*46.0);
                int loopBX=TIMELINE_SPLIT_X+(int)std::lround((loopB-timelineScrollSeconds)*46.0);
                HPEN lp=CreatePen(PS_SOLID,2,RGB(245,55,65)); HGDIOBJ oldLP=SelectObject(hdc,lp);
                HBRUSH lb=CreateSolidBrush(RGB(245,45,58)); HGDIOBJ oldLB=SelectObject(hdc,lb);
                if(loopAX>=TIMELINE_SPLIT_X && loopAX<=client.right){Ellipse(hdc,loopAX-6,rulerTop+2,loopAX+6,rulerTop+14); MoveToEx(hdc,loopAX,rulerTop+13,NULL);LineTo(hdc,loopAX,client.bottom);}
                if(loopBX>=TIMELINE_SPLIT_X && loopBX<=client.right){Ellipse(hdc,loopBX-6,rulerTop+2,loopBX+6,rulerTop+14); MoveToEx(hdc,loopBX,rulerTop+13,NULL);LineTo(hdc,loopBX,client.bottom);}
                SelectObject(hdc,oldLB);SelectObject(hdc,oldLP);DeleteObject(lb);DeleteObject(lp);
            }
            for(int row=timelineLayerScroll;row<(int)timelineLayers.size();++row){ int y=layerTop+(row-timelineLayerScroll)*alturaLinhaCamada; if(y+alturaLinhaCamada<95) continue; RECT lr{8,y,TIMELINE_SPLIT_X-2,y+alturaLinhaCamada-1}; COLORREF layerBase=timelineSegueTema?(temaAtual==2?RGB(70,25,45):temaAtual==1?RGB(228,230,235):temaAtual==3?RGB(31,18,44):RGB(37,40,48)):corTimelinePersonalizada; COLORREF layerSel=timelineSegueTema?(temaAtual==2?RGB(112,25,58):temaAtual==1?RGB(190,202,220):temaAtual==3?RGB(65,35,88):RGB(67,83,112)):RGB(std::min(255,GetRValue(corTimelinePersonalizada)+22),std::min(255,GetGValue(corTimelinePersonalizada)+22),std::min(255,GetBValue(corTimelinePersonalizada)+22)); HBRUSH br=CreateSolidBrush(row==selectedTimelineLayer?layerSel:layerBase); FillRect(hdc,&lr,br); DeleteObject(br); SetTextColor(hdc,timelineLayers[row].visible?RGB(235,238,245):RGB(130,130,130));
                HPEN pen=CreatePen(PS_SOLID,1,timelineLayers[row].visible?RGB(220,230,240):RGB(110,115,125)); HBRUSH hollow=(HBRUSH)GetStockObject(HOLLOW_BRUSH); HGDIOBJ oldPen=SelectObject(hdc,pen), oldBrush=SelectObject(hdc,hollow); Ellipse(hdc,12,y+(alturaLinhaCamada-13)/2,28,y+(alturaLinhaCamada+13)/2); Ellipse(hdc,17,y+(alturaLinhaCamada-7)/2,23,y+(alturaLinhaCamada+7)/2); SelectObject(hdc,oldPen); SelectObject(hdc,oldBrush); DeleteObject(pen);
                RECT nameRect{32,y,180,y+alturaLinhaCamada-1}; DrawTextW(hdc,timelineLayers[row].name.c_str(),-1,&nameRect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
                double secPos=0.0; int layerFrameNo=0; for(int fi=0;fi<(int)listaFrames.size();++fi){ if(!QuadroExisteNaCamada(listaFrames[fi],row)) continue; ++layerFrameNo; double dur=DuracaoCamadaQuadro(listaFrames[fi],row); int left=TIMELINE_SPLIT_X+(int)std::lround((secPos-timelineScrollSeconds)*46.0); int right=TIMELINE_SPLIT_X+(int)std::lround((secPos+dur-timelineScrollSeconds)*46.0); secPos+=dur; if(right<=TIMELINE_SPLIT_X || left>=frameHeader.right)continue; RECT cr{std::max(TIMELINE_SPLIT_X,left),y,std::min((int)frameHeader.right,right),y+alturaLinhaCamada-1}; if(cr.right<=cr.left) continue; COLORREF frameBase=timelineSegueTema?(temaAtual==2?RGB(78,34,53):temaAtual==1?RGB(210,215,225):temaAtual==3?RGB(38,25,50):RGB(43,46,54)):corTimelinePersonalizada; COLORREF frameSel=timelineSegueTema?(temaAtual==2?RGB(128,38,70):temaAtual==1?RGB(165,185,215):temaAtual==3?RGB(78,42,102):RGB(74,91,122)):RGB(std::min(255,GetRValue(corTimelinePersonalizada)+35),std::min(255,GetGValue(corTimelinePersonalizada)+35),std::min(255,GetBValue(corTimelinePersonalizada)+35)); HBRUSH cbr=CreateSolidBrush(fi==frameAtual?frameSel:frameBase); FillRect(hdc,&cr,cbr); DeleteObject(cbr); HPEN framePen=CreatePen(PS_SOLID,1,fi==frameAtual?RGB(145,178,224):RGB(82,88,101)); HGDIOBJ oldFP=SelectObject(hdc,framePen); HGDIOBJ oldFB=SelectObject(hdc,GetStockObject(HOLLOW_BRUSH)); Rectangle(hdc,cr.left,cr.top,cr.right,cr.bottom); SelectObject(hdc,oldFP); SelectObject(hdc,oldFB); DeleteObject(framePen); wchar_t frameLabel[20]; swprintf(frameLabel,20,L"%d",layerFrameNo); SetTextColor(hdc,RGB(238,241,248)); DrawTextW(hdc,frameLabel,-1,&cr,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX); if(row<(int)listaFrames[fi].layerBitmaps.size() && listaFrames[fi].layerBitmaps[row]){ SetTextColor(hdc,RGB(150,205,180)); RECT mark=cr; mark.left+=2; mark.top+=1; DrawTextW(hdc,L"◆",-1,&mark,DT_LEFT|DT_TOP|DT_SINGLELINE|DT_NOPREFIX); } }
            }
            // Faixa dedicada de música abaixo das camadas de desenho. O retângulo pode ser arrastado.
            const int audioLogicalRow=(int)timelineLayers.size();
            const int audioY=layerTop+(audioLogicalRow-timelineLayerScroll)*alturaLinhaCamada;
            if(audioY+alturaLinhaCamada>=95 && audioY<client.bottom) {
                RECT ar{8,audioY,TIMELINE_SPLIT_X-2,audioY+alturaLinhaCamada-1};
                HBRUSH ab=CreateSolidBrush(timelineSegueTema?(temaAtual==1?RGB(217,232,226):RGB(33,53,48)):RGB(36,78,70)); FillRect(hdc,&ar,ab); DeleteObject(ab);
                SetTextColor(hdc,RGB(228,245,238)); std::wstring audioLabel=KodaTraduzirTexto(L"Música"); DrawTextW(hdc,audioLabel.c_str(),-1,&ar,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
                for(const auto& asset:g_projectMediaAssets) if(asset.type==L"audio") {
                    double audDur=asset.durationSeconds>0.0?asset.durationSeconds:std::max(0.5,totalDuration-asset.startSeconds);
                    int aLeft=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds-timelineScrollSeconds)*46.0);
                    int aRight=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds+audDur-timelineScrollSeconds)*46.0);
                    RECT ac{std::max((int)TIMELINE_SPLIT_X,aLeft),audioY,std::min((int)client.right,aRight),audioY+alturaLinhaCamada-1};
                    if(ac.right<=ac.left) continue;
                    HBRUSH clip=CreateSolidBrush(RGB(43,132,103)); FillRect(hdc,&ac,clip); DeleteObject(clip);
                    HPEN ap=CreatePen(PS_SOLID,1,RGB(127,225,184)); HGDIOBJ oldap=SelectObject(hdc,ap); HGDIOBJ oldab=SelectObject(hdc,GetStockObject(HOLLOW_BRUSH)); Rectangle(hdc,ac.left,ac.top,ac.right,ac.bottom); SelectObject(hdc,oldap); SelectObject(hdc,oldab); DeleteObject(ap);
                    std::wstring base=asset.name.empty()?asset.relativePath:asset.name;
                    SetTextColor(hdc,RGB(250,255,250)); RECT txt=ac; txt.left+=5; DrawTextW(hdc,base.c_str(),-1,&txt,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
                }
            }
            // Faixa de vídeos e GIFs: clipes amarelos, posicionáveis com o mouse.
            const int visualLogicalRow=(int)timelineLayers.size()+1;
            const int visualY=layerTop+(visualLogicalRow-timelineLayerScroll)*alturaLinhaCamada;
            if(visualY+alturaLinhaCamada>=95 && visualY<client.bottom) {
                RECT vr{8,visualY,TIMELINE_SPLIT_X-2,visualY+alturaLinhaCamada-1};
                HBRUSH vbg=CreateSolidBrush(timelineSegueTema?(temaAtual==1?RGB(238,225,183):RGB(67,55,29)):RGB(67,55,29)); FillRect(hdc,&vr,vbg); DeleteObject(vbg);
                SetTextColor(hdc,RGB(255,231,159)); DrawTextW(hdc,L"Vídeo / GIF",-1,&vr,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
                for(const auto& asset:g_projectMediaAssets) {
                    if(asset.type!=L"video" && asset.type!=L"gif") continue;
                    double dur=asset.durationSeconds>0.0?asset.durationSeconds:3.0;
                    int left=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds-timelineScrollSeconds)*46.0);
                    int right=TIMELINE_SPLIT_X+(int)std::lround((asset.startSeconds+dur-timelineScrollSeconds)*46.0);
                    RECT ac{std::max((int)TIMELINE_SPLIT_X,left),visualY,std::min((int)client.right,std::max(left+12,right)),visualY+alturaLinhaCamada-1};
                    if(ac.right<=ac.left) continue;
                    HBRUSH clip=CreateSolidBrush(asset.type==L"gif"?RGB(190,151,44):RGB(174,126,28)); FillRect(hdc,&ac,clip); DeleteObject(clip);
                    HPEN vp=CreatePen(PS_SOLID,1,RGB(255,218,103)); HGDIOBJ oldvp=SelectObject(hdc,vp); HGDIOBJ oldvb=SelectObject(hdc,GetStockObject(HOLLOW_BRUSH)); Rectangle(hdc,ac.left,ac.top,ac.right,ac.bottom); SelectObject(hdc,oldvp); SelectObject(hdc,oldvb); DeleteObject(vp);
                    std::wstring base=asset.name.empty()?asset.relativePath:asset.name;
                    SetTextColor(hdc,RGB(255,250,225)); RECT txt=ac; txt.left+=5; DrawTextW(hdc,base.c_str(),-1,&txt,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
                }
            }
            std::wstring totalPrefix=KodaTraduzirTexto(L"Total");
            std::wstring frameWord=listaFrames.size()==1?KodaTraduzirTexto(L"quadro"):KodaTraduzirTexto(L"quadros");
            std::wstring totalText=totalPrefix+L": "+std::to_wstring(listaFrames.size())+L" "+frameWord+L" | "+FormatarTempoTotal(totalDuration);
            // O resumo fica na mesma faixa horizontal dos botões, alinhado à direita.
            SetTextColor(hdc,RGB(220,225,234));
            RECT totalRect{718,8,std::max<LONG>(718,client.right-10),34};
            DrawTextW(hdc,totalText.c_str(),-1,&totalRect,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
            // Pinça vertical de reprodução/posição: vermelho durante playback, ciano pausada.
            double playheadT=previewAtivo?previewTimeSeconds:timelinePlayheadSeconds;
            playheadT=std::max(0.0,std::min(totalDuration,playheadT));
            int playheadX=TIMELINE_SPLIT_X+(int)std::lround((playheadT-timelineScrollSeconds)*46.0);
            if(playheadX>=TIMELINE_SPLIT_X && playheadX<=client.right){
                HPEN phPen=CreatePen(PS_SOLID,2,previewAtivo?RGB(255,70,80):RGB(65,220,245));
                HGDIOBJ oldPh=SelectObject(hdc,phPen); MoveToEx(hdc,playheadX,rulerTop,NULL); LineTo(hdc,playheadX,client.bottom);
                POINT tri[3]={{playheadX-6,rulerTop},{playheadX+6,rulerTop},{playheadX,rulerTop+9}};
                HBRUSH phBrush=CreateSolidBrush(previewAtivo?RGB(255,70,80):RGB(65,220,245)); HGDIOBJ oldPhBrush=SelectObject(hdc,phBrush);
                Polygon(hdc,tri,3); SelectObject(hdc,oldPh); SelectObject(hdc,oldPhBrush); DeleteObject(phPen); DeleteObject(phBrush);
            }
            BitBlt(paintDC,0,0,bw,bh,hdc,0,0,SRCCOPY);
            SelectObject(hdc,oldBackBitmap); DeleteObject(backBitmap); DeleteDC(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK AplicarFonteControle(HWND child, LPARAM p) {
    SendMessageW(child, WM_SETFONT, (WPARAM)(HFONT)p, TRUE);
    return TRUE;
}

static BOOL CALLBACK CapturarGeometriaBaseTimeline(HWND child, LPARAM) {
    RECT r{}; GetWindowRect(child, &r); MapWindowPoints(NULL, GetParent(child), (POINT*)&r, 2);
    TimelineControlBaseRect item{}; item.hwnd=child; item.rect=r; g_timelineControlBaseRects.push_back(item); return TRUE;
}
static BOOL CALLBACK EscalarControleTimeline(HWND child, LPARAM param) {
    const int scale=(int)param;
    for(const auto& item:g_timelineControlBaseRects) if(item.hwnd==child) {
        const RECT& r=item.rect;
        SetWindowPos(child,NULL,MulDiv(r.left,scale,100),MulDiv(r.top,scale,100),
            std::max(10,MulDiv(r.right-r.left,scale,100)),std::max(10,MulDiv(r.bottom-r.top,scale,100)),SWP_NOZORDER|SWP_NOACTIVATE);
        break;
    }
    return TRUE;
}
void AplicarEscalaBotoesTimeline() {
    if(!hwndTimeline || !IsWindow(hwndTimeline)) return;
    if(g_timelineControlBaseRects.empty()) EnumChildWindows(hwndTimeline,CapturarGeometriaBaseTimeline,0);
    if(g_timelineControlBaseRects.empty()) return;
    EnumChildWindows(hwndTimeline,EscalarControleTimeline,(LPARAM)tamanhoBotoesTimeline);
    escalaBotoesTimelineAplicada=tamanhoBotoesTimeline; InvalidateRect(hwndTimeline,NULL,TRUE);
}
static void AplicarEscalaFerramentas() {
    if(!hwndTools || !IsWindow(hwndTools)) return;
    const int scale=(escalaInterface>0?escalaInterface:100), baseW=34,baseH=34,baseGap=5,x=3,y=5;
    const int w=std::max(24,MulDiv(baseW,scale,100)), h=std::max(24,MulDiv(baseH,scale,100));
    const int ids[]={ID_TOOL_LAPIS,ID_TOOL_BORRACHA,ID_TOOL_BALDE,ID_TOOL_LACON,ID_TOOL_RODA_CORES,ID_TOOL_CONTA_GOTAS,ID_BTN_RENDERIZAR};
    for(int i=0;i<7;++i){ HWND b=GetDlgItem(hwndTools,ids[i]); if(b) SetWindowPos(b,NULL,x,MulDiv(y+i*(baseH+baseGap),scale,100),w,h,SWP_NOZORDER|SWP_NOACTIVATE); }
    const int panelW=x+w+3, panelH=MulDiv(y+7*baseH+6*baseGap+5,scale,100);
    SetWindowPos(hwndTools,HWND_TOPMOST,12,70,panelW,panelH,SWP_NOACTIVATE|SWP_SHOWWINDOW); InvalidateRect(hwndTools,NULL,TRUE);
}

static void AnimarBotaoPreferencias(HWND parent,int id,COLORREF cor,bool encolher=false){
    HWND b=GetDlgItem(parent,id); if(!b)return; RECT r{};GetWindowRect(b,&r);MapWindowPoints(NULL,parent,(POINT*)&r,2);int w=r.right-r.left,h=r.bottom-r.top;int cx=r.left+w/2,cy=r.top+h/2;
    g_settingsAnimButtonId=id;g_settingsAnimColor=cor;InvalidateRect(b,NULL,TRUE);UpdateWindow(b);
    int delta=encolher?-5:7;SetWindowPos(b,NULL,cx-(w+delta)/2,cy-(h+delta/2)/2,w+delta,h+delta/2,SWP_NOZORDER|SWP_NOACTIVATE);InvalidateRect(b,NULL,TRUE);UpdateWindow(b);Sleep(55);
    SetWindowPos(b,NULL,r.left,r.top,w,h,SWP_NOZORDER|SWP_NOACTIVATE);InvalidateRect(b,NULL,TRUE);UpdateWindow(b);Sleep(35);
    g_settingsAnimButtonId=-1;InvalidateRect(b,NULL,TRUE);UpdateWindow(b);
    if(encolher){
        g_settingsAnimColor=cor; HWND blocks[4]={NULL,NULL,NULL,NULL}; int bx=cx-3,by=cy-3; int dx[4]={0,0,0,0},dy[4]={0,0,0,0}; for(int i=0;i<4;++i){dx[i]=(std::rand()%3)-1;dy[i]=(std::rand()%3)-1;if(dx[i]==0&&dy[i]==0)dx[i]=(i%2)?1:-1;}
        for(int i=0;i<4;++i)blocks[i]=CreateWindowW(L"STATIC",L"",WS_VISIBLE|WS_CHILD|SS_OWNERDRAW,bx,by,7,7,parent,(HMENU)(480+i),hInstGlobal,NULL);
        for(int step=1;step<=5;++step){for(int i=0;i<4;++i)if(blocks[i]&&IsWindow(blocks[i]))SetWindowPos(blocks[i],NULL,bx+dx[i]*step*7,by+dy[i]*step*7,std::max(1,7-step),std::max(1,7-step),SWP_NOZORDER|SWP_NOACTIVATE);UpdateWindow(parent);Sleep(18);}
        for(int i=0;i<4;++i)if(blocks[i])DestroyWindow(blocks[i]);
    }
}

static BOOL CALLBACK AplicarVisibilidadeCategoriaControle(HWND child, LPARAM param) {
    const int cat = (int)param;
    const int id = GetDlgCtrlID(child);
    bool show = false;

    // Navegação, cabeçalho, título da página, assinatura e botões gerais são fixos.
    if (id == 469 || id == ID_SETTINGS_HEADER ||
        id == ID_SETTINGS_CATEGORY_TITLE || id == ID_SETTINGS_CREATOR_LABEL) {
        show = true;
    } else if (id == ID_SETTINGS_SAVE || id == ID_SETTINGS_RESET) {
        show = cat != 5;
    } else {
        switch (cat) {
            case 0: // Interface
                show = id == ID_SETTINGS_THEME || id == ID_SETTINGS_UI_SCALE ||
                       id == ID_SETTINGS_TEXT_SIZE || id == ID_SETTINGS_LABEL_THEME ||
                       id == ID_SETTINGS_LABEL_UI_SCALE || id == ID_SETTINGS_LABEL_TEXT_SIZE;
                break;
            case 1: // Gráfico e desempenho
                show = id == ID_SETTINGS_RENDERER || id == ID_SETTINGS_CORES ||
                       id == ID_SETTINGS_RAM || id == ID_SETTINGS_LABEL_RENDERER ||
                       id == ID_SETTINGS_LABEL_CORES || id == ID_SETTINGS_LABEL_RAM ||
                       id == ID_SETTINGS_LABEL_SYSTEM || id == ID_SETTINGS_CPU_INFO ||
                       id == ID_SETTINGS_CPU_COUNT || id == ID_SETTINGS_CPU_MODE ||
                       (id >= ID_SETTINGS_CPU_MMX && id <= ID_SETTINGS_CPU_VIRTUAL);
                break;
            case 2: // Tablet
                show = id == ID_SETTINGS_PRESSURE_API || id == ID_SETTINGS_CURSOR ||
                       id == ID_SETTINGS_LABEL_PRESSURE_API || id == ID_SETTINGS_LABEL_TABLET_NOTE ||
                       id == ID_SETTINGS_TABLET_DEVICE_VALUE || id == ID_SETTINGS_TABLET_DRIVER_VALUE ||
                       id == ID_SETTINGS_TABLET_LEVELS_VALUE || id == ID_SETTINGS_TABLET_NAME_EDIT ||
                       id == ID_SETTINGS_TABLET_LEVELS_EDIT || id == ID_SETTINGS_TABLET_APPLY ||
                       id == ID_SETTINGS_TABLET_LABEL_DEVICE || id == ID_SETTINGS_TABLET_LABEL_DRIVER ||
                       id == ID_SETTINGS_TABLET_LABEL_LEVELS || id == ID_SETTINGS_TABLET_LABEL_MANUAL_NAME ||
                       id == ID_SETTINGS_TABLET_LABEL_MANUAL_LEVELS || id == ID_SETTINGS_TABLET_NOTE;
                break;
            case 3: // Timeline
                show = id == ID_SETTINGS_TIMELINE_BUTTONS || id == ID_SETTINGS_LAYER_HEIGHT ||
                       id == ID_SETTINGS_RULER_HEIGHT || id == ID_SETTINGS_LABEL_TIMELINE_BUTTONS ||
                       id == ID_SETTINGS_LABEL_LAYER_HEIGHT || id == ID_SETTINGS_LABEL_RULER_HEIGHT;
                break;
            case 4: // Histórico
                show = id == ID_SETTINGS_HISTORY_ACTIONS || id == ID_SETTINGS_HISTORY_CACHE ||
                       id == ID_SETTINGS_LABEL_HISTORY_ACTIONS || id == ID_SETTINGS_LABEL_HISTORY_CACHE ||
                       id == ID_SETTINGS_LABEL_HISTORY_NOTE || id == ID_SETTINGS_LABEL_CONFIG_PATH;
                break;
            case 5: // Atalhos ficam incorporados à página de preferências
                show = id == ID_SETTINGS_LABEL_SHORTCUT_NOTE ||
                       id == ID_SETTINGS_HOTKEY_LIST || id == ID_SETTINGS_HOTKEY_KEY ||
                       id == ID_SETTINGS_HOTKEY_APPLY || id == ID_SETTINGS_HOTKEY_SAVE ||
                       id == ID_SETTINGS_HOTKEY_RESET;
                break;
            case 6: // Guia completo com rolagem vertical
                show = id == ID_SETTINGS_EXPLANATION_TEXT;
                break;
        }
    }
    ShowWindow(child, show ? SW_SHOW : SW_HIDE);
    return TRUE;
}

static void AplicarCategoriaPreferencias(HWND hwnd, int categoria) {
    if (categoria < 0 || categoria > 6) categoria = 0;
    HWND titulo = GetDlgItem(hwnd, ID_SETTINGS_CATEGORY_TITLE);
    const wchar_t* titulos[] = {
        L"Interface", L"Gráfico e desempenho", L"Tablet", L"Timeline", L"Histórico", L"Atalhos", L"Explicação"
    };
    if (titulo) {
        std::wstring title=(categoria==6)?KodaRotuloExplicacao(true):KodaTraduzirTexto(titulos[categoria]);
        // O texto da aba é destacado em maiúsculas nos idiomas latinos.
        if(categoria!=6) { for(size_t i=0;i<title.size();++i) if(title[i]>=L'a'&&title[i]<=L'z') title[i]=(wchar_t)(title[i]-L'a'+L'A'); }
        SetWindowTextW(titulo, title.c_str());
    }
    EnumChildWindows(hwnd, AplicarVisibilidadeCategoriaControle, (LPARAM)categoria);
}

static bool KodaCPUFeatureControlAtivo(int id) {
    if(id==ID_SETTINGS_CPU_AVX) return g_kodaCPU.avxUsable; // geometria do anti-alias do canvas
    if(id==ID_SETTINGS_CPU_AVX2) return g_kodaCPU.pixelKernel==2; // conversão de pixels
    if(id==ID_SETTINGS_CPU_SSE2) return g_kodaCPU.pixelKernel==1; // conversão de pixels / fallback
    return false;
}
static std::wstring KodaCPUFeatureTexto(const wchar_t* name,bool supported,bool active) {
    std::wstring t=name; t+=L": "; t+=supported?L"suportada":L"não disponível";
    if(active) t+=L" — ATIVA";
    return t;
}
static void AtualizarPainelCPU(HWND hwnd) {
    if(!hwnd||!IsWindow(hwnd)) return;
    std::wstring cpu=L"CPU: "; cpu+=g_kodaCPU.brand; cpu+=L"  ["; cpu+=g_kodaCPU.vendor; cpu+=L"]";
    SetWindowTextW(GetDlgItem(hwnd,ID_SETTINGS_CPU_INFO),cpu.c_str());
    wchar_t summary[180];
    swprintf(summary,180,L"Conjuntos/recursos detectados nesta lista: %d de 11 | verificados ao iniciar",g_kodaCPU.detectedCount);
    SetWindowTextW(GetDlgItem(hwnd,ID_SETTINGS_CPU_COUNT),summary);
    std::wstring modeText=L"Conversão de imagem: ";
    modeText+=g_kodaCPU.pixelKernel==2?L"AVX2 (8 pixels/lote)":g_kodaCPU.pixelKernel==1?L"SSE2 (4 pixels/lote)":L"escalar";
    modeText+=L" | pincel anti-alias do canvas: ";
    modeText+=g_kodaCPU.avxUsable?L"AVX (4 pixels/lote)":g_kodaCPU.sse2?L"SSE2 (2 pixels/lote)":L"escalar";
    SetWindowTextW(GetDlgItem(hwnd,ID_SETTINGS_CPU_MODE),modeText.c_str());
    struct Row {int id; const wchar_t* name; bool supported;};
    const Row rows[]={
        {ID_SETTINGS_CPU_MMX,L"MMX",g_kodaCPU.mmx},
        {ID_SETTINGS_CPU_SSE,L"SSE",g_kodaCPU.sse},
        {ID_SETTINGS_CPU_SSE2,L"SSE2",g_kodaCPU.sse2},
        {ID_SETTINGS_CPU_SSE3,L"SSE3",g_kodaCPU.sse3},
        {ID_SETTINGS_CPU_SSSE3,L"SSSE3",g_kodaCPU.ssse3},
        {ID_SETTINGS_CPU_SSE41,L"SSE4.1",g_kodaCPU.sse41},
        {ID_SETTINGS_CPU_SSE42,L"SSE4.2",g_kodaCPU.sse42},
        {ID_SETTINGS_CPU_AVX,L"AVX",g_kodaCPU.avxUsable},
        {ID_SETTINGS_CPU_AVX2,L"AVX2",g_kodaCPU.avx2Usable},
        {ID_SETTINGS_CPU_AES,L"AES",g_kodaCPU.aes},
        {ID_SETTINGS_CPU_VIRTUAL,L"VT-x / AMD-V",(g_kodaCPU.vmx||g_kodaCPU.svm)}
    };
    for(size_t i=0;i<sizeof(rows)/sizeof(rows[0]);++i) {
        const bool ativo=KodaCPUFeatureControlAtivo(rows[i].id);
        std::wstring t=KodaCPUFeatureTexto(rows[i].name,rows[i].supported,ativo);
        if(ativo && rows[i].id==ID_SETTINGS_CPU_AVX) t+=L" (cálculo do pincel)";
        else if(ativo && (rows[i].id==ID_SETTINGS_CPU_AVX2 || rows[i].id==ID_SETTINGS_CPU_SSE2)) t+=L" (conversão de imagem)";
        if(rows[i].id==ID_SETTINGS_CPU_VIRTUAL && rows[i].supported) {
            t=L""; t+=g_kodaCPU.vmx?L"Intel VT-x: CPU suporta":L"AMD-V: CPU suporta";
            t+=L" (virtualização, não desenho)";
        }
        if(rows[i].id==ID_SETTINGS_CPU_AES && rows[i].supported) t+=L" (não usada no canvas)";
        if((rows[i].id==ID_SETTINGS_CPU_AVX || rows[i].id==ID_SETTINGS_CPU_AVX2) &&
            (rows[i].id==ID_SETTINGS_CPU_AVX?!g_kodaCPU.avxUsable:!g_kodaCPU.avx2Usable)) {
            if((rows[i].id==ID_SETTINGS_CPU_AVX&&g_kodaCPU.avx)||(rows[i].id==ID_SETTINGS_CPU_AVX2&&g_kodaCPU.avx2))
                t=std::wstring(rows[i].name)+L": CPU suporta, mas o SO não habilitou XMM/YMM";
        }
        SetWindowTextW(GetDlgItem(hwnd,rows[i].id),t.c_str());
    }
}

LRESULT CALLBACK WndProcSettings(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(msg==WM_ERASEBKGND){HDC dc=(HDC)wParam;RECT rc{};GetClientRect(hwnd,&rc);COLORREF bg=temaAtual==1?RGB(245,245,245):temaAtual==2?RGB(52,24,43):temaAtual==3?RGB(24,18,38):RGB(30,30,34);HBRUSH b=CreateSolidBrush(bg);FillRect(dc,&rc,b);DeleteObject(b);return 1;}
    if(msg==WM_DRAWITEM){ DRAWITEMSTRUCT* d=(DRAWITEMSTRUCT*)lParam; if(d && d->CtlID>=480 && d->CtlID<=483){HBRUSH b=CreateSolidBrush(g_settingsAnimColor);FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);return TRUE;} if(d && (d->CtlID==ID_SETTINGS_CURSOR||d->CtlID==ID_SETTINGS_SAVE||d->CtlID==ID_SETTINGS_RESET)){
        COLORREF bg=(d->CtlID==g_settingsAnimButtonId)?g_settingsAnimColor:(temaAtual==1?RGB(225,225,230):temaAtual==2?RGB(73,36,64):temaAtual==3?RGB(45,32,68):RGB(55,55,62));
        HBRUSH br=CreateSolidBrush(bg);FillRect(d->hDC,&d->rcItem,br);DeleteObject(br);FrameRect(d->hDC,&d->rcItem,(HBRUSH)GetStockObject(GRAY_BRUSH));SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,(temaAtual==1)?RGB(25,25,25):RGB(245,245,250));wchar_t t[128]={0};GetWindowTextW(d->hwndItem,t,127);DrawTextW(d->hDC,t,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);return TRUE;
    }}
    if(msg==WM_CTLCOLORDLG){ HDC dc=(HDC)wParam; static HBRUSH db[4]={NULL,NULL,NULL,NULL}; int ix=temaAtual<0||temaAtual>3?0:temaAtual; COLORREF bg=ix==1?RGB(245,245,245):ix==2?RGB(52,24,43):ix==3?RGB(24,18,38):RGB(30,30,34); SetBkColor(dc,bg); if(!db[ix])db[ix]=CreateSolidBrush(bg); return (LRESULT)db[ix]; }
    if(msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLOREDIT || msg==WM_CTLCOLORLISTBOX || msg==WM_CTLCOLORBTN){
        HDC dc=(HDC)wParam; bool light=(temaAtual==1);
        COLORREF bg=light?RGB(245,245,245):temaAtual==2?RGB(52,24,43):temaAtual==3?RGB(24,18,38):RGB(30,30,34);
        COLORREF fg=light?RGB(30,30,30):RGB(225,225,232);
        if(msg==WM_CTLCOLORSTATIC && KodaCPUFeatureControlAtivo(GetDlgCtrlID((HWND)lParam)))
            fg=light?RGB(0,130,45):RGB(80,235,115);
        SetBkColor(dc,bg); SetTextColor(dc,fg);
        static HBRUSH brushes[4]={NULL,NULL,NULL,NULL}; int ix=temaAtual<0||temaAtual>3?0:temaAtual;
        if(!brushes[ix])brushes[ix]=CreateSolidBrush(bg); return (LRESULT)brushes[ix];
    }
    if (msg == WM_MOUSEMOVE) { settingsMouseEntered = true; }
    if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id == 469 && HIWORD(wParam) == LBN_SELCHANGE) {
            int cat = (int)SendMessageW(GetDlgItem(hwnd, 469), LB_GETCURSEL, 0, 0);
            AplicarCategoriaPreferencias(hwnd, cat);
            int target = cat == 0 ? ID_SETTINGS_THEME : cat == 1 ? ID_SETTINGS_RENDERER :
                         cat == 2 ? ID_SETTINGS_PRESSURE_API : cat == 3 ? ID_SETTINGS_TIMELINE_BUTTONS :
                         cat == 4 ? ID_SETTINGS_HISTORY_ACTIONS : cat == 5 ? ID_SETTINGS_LABEL_SHORTCUT_NOTE : ID_SETTINGS_EXPLANATION_TEXT;
            HWND h = GetDlgItem(hwnd, target);
            if (cat != 5 && h && IsWindowVisible(h)) SetFocus(h);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_LIST), LB_GETCURSEL, 0, 0);
            AtualizarListaAtalhos(hwnd, sel);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_APPLY) {
            int sel = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_LIST), LB_GETCURSEL, 0, 0);
            int ki = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_KEY), CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_kodaShortcutCount && !g_kodaShortcuts[sel].fixed && ki >= 0) {
                UINT candidate = (UINT)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_KEY), CB_GETITEMDATA, ki, 0);
                bool duplicate = candidate == VK_F11;
                for (int j = 0; j < g_kodaShortcutCount; ++j)
                    if (j != sel && !g_kodaShortcuts[j].fixed && g_kodaShortcuts[j].vk == candidate) duplicate = true;
                if (duplicate) {
                    MessageBoxW(hwnd, L"Essa tecla já está reservada ou em uso por outra função. Escolha outra tecla.", L"Atalho indisponível", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                g_kodaShortcuts[sel].vk = candidate;
                g_selectedShortcut = sel;
            }
            AtualizarListaAtalhos(hwnd, g_selectedShortcut);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_SAVE) {
            AnimarBotaoPreferencias(hwnd, id, RGB(45,190,95));
            SalvarConfiguracoesUsuario();
            MessageBoxW(hwnd, L"Todos os atalhos configuráveis foram salvos.", L"Kodanima", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_RESET) {
            AnimarBotaoPreferencias(hwnd, id, RGB(220,55,65), true);
            for (int i = 0; i < g_kodaShortcutCount; ++i)
                if (!g_kodaShortcuts[i].fixed) g_kodaShortcuts[i].vk = g_kodaShortcuts[i].defaultVk;
            SalvarConfiguracoesUsuario();
            AtualizarListaAtalhos(hwnd, 0);
            return 0;
        }
        if (id == ID_SETTINGS_CURSOR) {
            AnimarBotaoPreferencias(hwnd,id,RGB(145,75,220));
            OPENFILENAMEW ofn{}; wchar_t file[MAX_PATH] = L"";
            ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hwndSettings; ofn.lpstrFilter=L"Cursores (*.cur;*.ani)\0*.cur;*.ani\0Todos os arquivos\0*.*\0"; ofn.lpstrFile=file; ofn.nMaxFile=MAX_PATH; ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) { HCURSOR c=LoadCursorFromFileW(file); if(c){ if(cursorPersonalizado) DestroyCursor(cursorPersonalizado); cursorPersonalizado=c; lstrcpynW(caminhoCursor,file,MAX_PATH); if(hwndMain) AplicarCursor(hwndMain); if(hwndCanvas) AplicarCursor(hwndCanvas); if(hwndTools) AplicarCursor(hwndTools); if(hwndTimeline) AplicarCursor(hwndTimeline); if(hwndShortcutPanel) AplicarCursor(hwndShortcutPanel); if(pressureApi==0 && g_wintabContext) AplicarCursorGlobalWintab(); } }
            return 0;
        }
        if(id==ID_SETTINGS_TABLET_APPLY) {
            wchar_t name[MAX_PATH]={0}, levelsText[64]={0};
            GetWindowTextW(GetDlgItem(hwndSettings,ID_SETTINGS_TABLET_NAME_EDIT),name,MAX_PATH);
            GetWindowTextW(GetDlgItem(hwndSettings,ID_SETTINGS_TABLET_LEVELS_EDIT),levelsText,64);
            wchar_t* end=NULL; long parsed=0;
            if(levelsText[0]) { parsed=wcstol(levelsText,&end,10); while(end && (*end==L' '||*end==L'\t')) ++end; if(end==levelsText || (end && *end) || parsed<0 || parsed==1 || parsed>65536) {
                MessageBoxW(hwndSettings,L"Informe 0 para automático ou um número de níveis entre 2 e 65536.",L"Perfil de mesa",MB_OK|MB_ICONWARNING); return 0;
            }}
            g_tabletManualDeviceName=name;
            g_tabletManualPressureLevels=(int)parsed;
            if(!g_wintabPressureAxisAvailable && g_tabletManualPressureLevels==0) {
                std::wstring profileSource; int profileLevels=InferirNiveisPerfilTablet(
                    g_tabletManualDeviceName.empty()?g_tabletDetectedName:g_tabletManualDeviceName,profileSource);
                if(profileLevels>=2) { g_tabletPressureLevelsDetected=profileLevels; g_tabletPressureLevelsSource=profileSource; g_tabletLevelsAreEstimated=true; }
                else { g_tabletPressureLevelsDetected=0; g_tabletPressureLevelsSource=L"Níveis não informados pelo driver; use perfil manual"; g_tabletLevelsAreEstimated=false; }
            }
            SalvarConfiguracoesUsuario(); AtualizarPainelTablet();
            MessageBoxW(hwndSettings,L"Perfil da mesa salvo em config.ini. O eixo físico Wintab mantém a resolução real; o valor manual calibra/limita a pressão quando necessário.",L"Perfil de mesa",MB_OK|MB_ICONINFORMATION);
            return 0;
        }
        if (id == ID_SETTINGS_SAVE) {
            AnimarBotaoPreferencias(hwnd,id,RGB(45,190,95));
            temaAtual=(int)SendMessageW(hwndComboTema,CB_GETCURSEL,0,0); if(temaAtual<0||temaAtual>3) temaAtual=0;
            int motorSel=(int)SendMessageW(hwndComboRenderer,CB_GETCURSEL,0,0); rendererPreferido=motorSel>=0?(int)SendMessageW(hwndComboRenderer,CB_GETITEMDATA,motorSel,0):0; if(rendererPreferido<0||rendererPreferido>3)rendererPreferido=0; DestruirDirectX9(); DestruirDirectX11(); DestruirDirectX12(); g_dx9FalhaAvisada=false;g_dx11FalhaAvisada=false;g_dx12FalhaAvisada=false;
            int uiSel=(int)SendMessageW(hwndComboUIScale,CB_GETCURSEL,0,0); escalaInterface=uiSel==0?80:uiSel==2?125:uiSel==3?150:100;
            int txtSel=(int)SendMessageW(hwndComboTextSize,CB_GETCURSEL,0,0); tamanhoTexto=txtSel==0?80:txtSel==2?125:txtSel==3?150:100;
            HWND comboTB=GetDlgItem(hwndSettings,ID_SETTINGS_TIMELINE_BUTTONS); int tbSel=(int)SendMessageW(comboTB,CB_GETCURSEL,0,0); tamanhoBotoesTimeline=tbSel==0?70:tbSel==2?90:tbSel==3?100:80;
            int lh=(int)SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_LAYER_HEIGHT),CB_GETCURSEL,0,0); alturaLinhaCamada=lh==0?22:lh==2?32:lh==3?38:27;
            int rh=(int)SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_RULER_HEIGHT),CB_GETCURSEL,0,0); alturaReguaTimeline=rh==0?18:rh==2?30:rh==3?36:24;
            int apiSel=(int)SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_PRESSURE_API),CB_GETCURSEL,0,0); pressureApi=(apiSel==1)?1:0;
            int historySel=(int)SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_HISTORY_ACTIONS),CB_GETCURSEL,0,0);
            historyActionLimit=historySel==0?5:historySel==1?10:historySel==3?30:historySel==4?50:20;
            int historyCacheSel=(int)SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_HISTORY_CACHE),CB_GETCURSEL,0,0);
            historyCacheLimitMB=historyCacheSel==0?16:historyCacheSel==1?32:historyCacheSel==2?64:historyCacheSel==3?128:200;
            AplicarLimitesHistoricoTimeline();
            for(auto& fr:listaFrames) AplicarLimitesHistoricoFrame(fr);
            FecharWintab(); if(pressureApi==0 && hwndCanvas && IsWindow(hwndCanvas)) InicializarWintab(hwndCanvas);
            AtualizarPainelTablet();
// Mantém a preferência escolhida; se o motor falhar, o canvas informa o erro sem trocar silenciosamente para GDI.
            int coreSel=(int)SendMessageW(hwndComboCores,CB_GETCURSEL,0,0); nucleosDesejados=coreSel<=0?0:coreSel;
            int ramSel=(int)SendMessageW(hwndComboRAM,CB_GETCURSEL,0,0); limiteRAMMB=ramSel==1?512:ramSel==2?1024:ramSel==3?2048:ramSel==4?4096:ramSel==5?8192:0;
            bool cpuOk=AplicarLimiteNucleos(nucleosDesejados);
            bool ramOk=AplicarLimiteRAM(limiteRAMMB);
            // Aplica o tamanho do texto aos controles das janelas abertas.
            if (fontePreferencias) { DeleteObject(fontePreferencias); fontePreferencias=NULL; }
            LOGFONTW lf{}; lf.lfHeight=-((12*tamanhoTexto+50)/100); lf.lfWeight=FW_NORMAL; lstrcpyW(lf.lfFaceName,L"Segoe UI");
            fontePreferencias=CreateFontIndirectW(&lf);
            if (fontePreferencias) {
                if (hwndMain) EnumChildWindows(hwndMain, AplicarFonteControle, (LPARAM)fontePreferencias);
                if (hwndCanvas) EnumChildWindows(hwndCanvas, AplicarFonteControle, (LPARAM)fontePreferencias);
                if (hwndTools) EnumChildWindows(hwndTools, AplicarFonteControle, (LPARAM)fontePreferencias);
                if (hwndTimeline) EnumChildWindows(hwndTimeline, AplicarFonteControle, (LPARAM)fontePreferencias);
                if (hwndSettings) EnumChildWindows(hwndSettings, AplicarFonteControle, (LPARAM)fontePreferencias);
                if (hwndShortcutPanel) EnumChildWindows(hwndShortcutPanel, AplicarFonteControle, (LPARAM)fontePreferencias);
            }
            // A escala da interface altera apenas os controles da barra de ferramentas.
            // Canvas e timeline não mudam de tamanho ao salvar qualquer preferência.
            AplicarEscalaFerramentas();
            escalaAplicada=escalaInterface;
            SalvarConfiguracoesUsuario();
            if (hwndSettings) InvalidateRect(hwndSettings, NULL, TRUE);
            if (hwndShortcutPanel) InvalidateRect(hwndShortcutPanel, NULL, TRUE);
            AplicarEscalaBotoesTimeline();
            InvalidarCacheComposicaoCanvas(); if(hwndMain) AplicarCursor(hwndMain); if(hwndTools) AplicarCursor(hwndTools); if(hwndTimeline) AplicarCursor(hwndTimeline); if(hwndShortcutPanel) AplicarCursor(hwndShortcutPanel); if(hwndCanvas) { SetClassLongPtrW(hwndCanvas,GCLP_HBRBACKGROUND,(LONG_PTR)CreateSolidBrush(CorTemaFundo())); AplicarCursor(hwndCanvas); InvalidateRect(hwndCanvas,NULL,TRUE); }
            MessageBoxW(hwndSettings,L"Configurações salvas em Documentos\\Kodanima\\config.ini",L"Kodanima",MB_OK|MB_ICONINFORMATION); return 0;
        }
        if (id == ID_SETTINGS_RESET) {
            AnimarBotaoPreferencias(hwnd,id,RGB(220,55,65),true);
            temaAtual=0; rendererPreferido=DetectarMelhorMotorGrafico(); escalaInterface=100; tamanhoTexto=100;
            tamanhoBotoesTimeline=80; alturaLinhaCamada=27; alturaReguaTimeline=24; timelineLayerScroll=0; nucleosDesejados=0; limiteRAMMB=0;
            tamanhoLapis=2.0f; lapisAntiAlias=true; durezaPincel=3; nivelGapBalde=8; toleranciaBalde=3; pressaoCanetaAtiva=false; pressureApi=0; tamanhoMinimoPressao=0.01f; tamanhoMaximoPressao=12.0f; g_tabletManualPressureLevels=0; g_tabletManualDeviceName.clear(); historyActionLimit=20; historyCacheLimitMB=200; AplicarLimitesHistoricoTimeline(); for(auto& fr:listaFrames) AplicarLimitesHistoricoFrame(fr); FecharWintab(); if(hwndCanvas && IsWindow(hwndCanvas)) InicializarWintab(hwndCanvas);
            if(hwndComboPressureAPI) SendMessageW(hwndComboPressureAPI,CB_SETCURSEL,0,0);
            SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_HISTORY_ACTIONS),CB_SETCURSEL,2,0);
            SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_HISTORY_CACHE),CB_SETCURSEL,4,0);
            caminhoCursor[0]=0;
            if(cursorPersonalizado){DestroyCursor(cursorPersonalizado);cursorPersonalizado=NULL;}
            SendMessageW(hwndComboTema,CB_SETCURSEL,0,0);
            { int best=-1; for(int i=0;i<(int)SendMessageW(hwndComboRenderer,CB_GETCOUNT,0,0);++i) if((int)SendMessageW(hwndComboRenderer,CB_GETITEMDATA,i,0)==rendererPreferido) { best=i; break; } SendMessageW(hwndComboRenderer,CB_SETCURSEL,best>=0?best:0,0); }
            SendMessageW(hwndComboUIScale,CB_SETCURSEL,1,0);
            SendMessageW(hwndComboTextSize,CB_SETCURSEL,1,0);
            SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_TIMELINE_BUTTONS),CB_SETCURSEL,1,0);
            SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_LAYER_HEIGHT),CB_SETCURSEL,1,0);
            SendMessageW(GetDlgItem(hwndSettings,ID_SETTINGS_RULER_HEIGHT),CB_SETCURSEL,1,0);
            SendMessageW(hwndComboCores,CB_SETCURSEL,0,0); SendMessageW(hwndComboRAM,CB_SETCURSEL,0,0);
            AplicarLimiteNucleos(0); AplicarLimiteRAM(0);
            escalaAplicada=100; SalvarConfiguracoesUsuario(); AtualizarPainelTablet(); AplicarEscalaFerramentas(); AplicarEscalaBotoesTimeline(); if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,TRUE);
            InvalidarCacheComposicaoCanvas(); if(hwndMain) AplicarCursor(hwndMain); if(hwndTools) AplicarCursor(hwndTools); if(hwndTimeline) AplicarCursor(hwndTimeline); if(hwndShortcutPanel) AplicarCursor(hwndShortcutPanel); if(hwndCanvas && IsWindow(hwndCanvas)) { SetClassLongPtrW(hwndCanvas,GCLP_HBRBACKGROUND,(LONG_PTR)CreateSolidBrush(CorTemaFundo())); AplicarCursor(hwndCanvas); InvalidateRect(hwndCanvas,NULL,TRUE); }
            MessageBoxW(hwndSettings,L"Preferências redefinidas para o padrão de fábrica e salvas no config.ini.",L"Kodanima",MB_OK|MB_ICONINFORMATION);
            return 0;
        }
    }

    if(msg==WM_CLOSE){ hwndSettings=NULL; DestroyWindow(hwnd); return 0; }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}

static void AtualizarListaAtalhos(HWND hwnd, int indiceSelecionado) {
    HWND lista = GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_LIST);
    if (!lista || !IsWindow(lista)) return;
    SendMessageW(lista, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_kodaShortcutCount; ++i) {
        wchar_t row[192];
        std::wstring key;
        if(g_kodaShortcuts[i].ctrl) key+=L"Ctrl+";
        if(g_kodaShortcuts[i].shift) key+=L"Shift+";
        key+=NomeTeclaKoda(g_kodaShortcuts[i].vk);
        if(g_kodaShortcuts[i].fixed) key+=L" (fixo)";
        wsprintfW(row, L"%s    —    %s", g_kodaShortcuts[i].name, key.c_str());
        SendMessageW(lista, LB_ADDSTRING, 0, (LPARAM)row);
    }
    if (g_kodaShortcutCount > 0) {
        if (indiceSelecionado < 0 || indiceSelecionado >= g_kodaShortcutCount) indiceSelecionado = 0;
        SendMessageW(lista, LB_SETCURSEL, indiceSelecionado, 0);
        g_selectedShortcut = indiceSelecionado;
    }
    HWND combo = GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_KEY);
    if (combo && g_selectedShortcut >= 0 && g_selectedShortcut < g_kodaShortcutCount) {
        for (int k = 0; k < (int)SendMessageW(combo, CB_GETCOUNT, 0, 0); ++k) {
            if ((UINT)SendMessageW(combo, CB_GETITEMDATA, k, 0) == g_kodaShortcuts[g_selectedShortcut].vk) {
                SendMessageW(combo, CB_SETCURSEL, k, 0);
                break;
            }
        }
        EnableWindow(combo, !g_kodaShortcuts[g_selectedShortcut].fixed);
        EnableWindow(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_APPLY), !g_kodaShortcuts[g_selectedShortcut].fixed);
    }
}

void AbrirPainelAtalhos() {
    if (!hwndCanvas || !IsWindow(hwndCanvas)) return;
    if (hwndShortcutPanel && IsWindow(hwndShortcutPanel)) {
        ShowWindow(hwndShortcutPanel, SW_SHOW);
        SetWindowPos(hwndShortcutPanel, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProcShortcuts;
    wc.hInstance = hInstGlobal;
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wc.hIcon = hIconeApp;
    wc.hIconSm = hIconeApp;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"KodanimaShortcutPanelClass";
    RegisterClassExW(&wc);

    const int panelW = 292, panelH = 488;
    RECT canvasRect{};
    GetWindowRect(hwndCanvas, &canvasRect);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = canvasRect.right + 8;
    int y = canvasRect.top;
    int actualCanvasW = canvasRect.right - canvasRect.left;
    if (x + panelW > screenW) {
        int fitCanvasW = screenW - canvasRect.left - panelW - 16;
        if (fitCanvasW >= 320) {
            SetWindowPos(hwndCanvas, NULL, canvasRect.left, canvasRect.top,
                         fitCanvasW, canvasRect.bottom - canvasRect.top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            actualCanvasW = fitCanvasW;
            x = canvasRect.left + actualCanvasW + 8;
        } else {
            x = std::max(0, screenW - panelW - 4);
        }
    }
    if (y + panelH > screenH) y = std::max(0, screenH - panelH - 4);

    hwndShortcutPanel = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"KodanimaShortcutPanelClass",
        L"Atalhos do Kodanima",
        WS_OVERLAPPED | WS_CAPTION | WS_CLIPCHILDREN,
        x, y, panelW, panelH, NULL, NULL, hInstGlobal, NULL);
    if (!hwndShortcutPanel) return;

    CreateWindowW(L"STATIC", L"Funções e teclas", WS_VISIBLE | WS_CHILD,
        12, 12, 258, 20, hwndShortcutPanel, NULL, hInstGlobal, NULL);
    HWND hotList = CreateWindowW(L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
        12, 36, 258, 248, hwndShortcutPanel,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_LIST, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"Nova tecla:", WS_VISIBLE | WS_CHILD,
        12, 296, 130, 22, hwndShortcutPanel, NULL, hInstGlobal, NULL);
    HWND keyCombo = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        12, 320, 132, 180, hwndShortcutPanel,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_KEY, hInstGlobal, NULL);
    const wchar_t* keys[] = {
        L",", L".", L"Enter", L"Tab", L"Delete", L"F1", L"F2", L"F3", L"F4", L"F5",
        L"F6", L"F7", L"F8", L"F9", L"F10", L"F11", L"F12",
        L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L",
        L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z",
        L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
        L"Home", L"End", L"PageUp", L"PageDown"
    };
    for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); ++i) {
        int n = (int)SendMessageW(keyCombo, CB_ADDSTRING, 0, (LPARAM)keys[i]);
        SendMessageW(keyCombo, CB_SETITEMDATA, n, VkPorIndiceTeclaKoda(i));
    }
    CreateWindowW(L"BUTTON", L"Aplicar", WS_VISIBLE | WS_CHILD,
        153, 320, 117, 30, hwndShortcutPanel,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_APPLY, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"Salvar atalhos",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
        153, 360, 117, 32, hwndShortcutPanel,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_SAVE, hInstGlobal, NULL);
    CreateWindowW(L"BUTTON", L"Restaurar padrão",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        12, 360, 132, 32, hwndShortcutPanel,
        (HMENU)(INT_PTR)ID_SETTINGS_HOTKEY_RESET, hInstGlobal, NULL);
    CreateWindowW(L"STATIC", L"As mudanças aplicadas ficam ativas durante a edição.",
        WS_VISIBLE | WS_CHILD, 12, 402, 258, 38, hwndShortcutPanel, NULL, hInstGlobal, NULL);

    AtualizarListaAtalhos(hwndShortcutPanel, g_selectedShortcut);
    if (fontePreferencias)
        EnumChildWindows(hwndShortcutPanel, AplicarFonteControle, (LPARAM)fontePreferencias);
    AplicarCursor(hwndShortcutPanel);
    ShowWindow(hwndShortcutPanel, SW_SHOW);
    SetWindowPos(hwndShortcutPanel, HWND_TOPMOST, x, y, panelW, panelH,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
    (void)hotList;
}

LRESULT CALLBACK WndProcShortcuts(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_ERASEBKGND) {
        HDC dc = (HDC)wParam; RECT rc{}; GetClientRect(hwnd, &rc);
        COLORREF bg = temaAtual == 1 ? RGB(245,245,245) : temaAtual == 2 ? RGB(52,24,43) : temaAtual == 3 ? RGB(24,18,38) : RGB(30,30,34);
        HBRUSH b = CreateSolidBrush(bg); FillRect(dc, &rc, b); DeleteObject(b); return 1;
    }
    if (msg == WM_CTLCOLORDLG || msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT ||
        msg == WM_CTLCOLORLISTBOX || msg == WM_CTLCOLORBTN) {
        HDC dc = (HDC)wParam;
        bool light = temaAtual == 1;
        COLORREF bg = light ? RGB(245,245,245) : temaAtual == 2 ? RGB(52,24,43) : temaAtual == 3 ? RGB(24,18,38) : RGB(30,30,34);
        COLORREF fg = light ? RGB(30,30,30) : RGB(225,225,232);
        SetBkColor(dc, bg); SetTextColor(dc, fg);
        static HBRUSH brushes[4] = { NULL, NULL, NULL, NULL };
        int ix = temaAtual < 0 || temaAtual > 3 ? 0 : temaAtual;
        if (!brushes[ix]) brushes[ix] = CreateSolidBrush(bg);
        return (LRESULT)brushes[ix];
    }
    if (msg == WM_DRAWITEM) {
        DRAWITEMSTRUCT* d = (DRAWITEMSTRUCT*)lParam;
        if (d && d->CtlID >= 480 && d->CtlID <= 483) {
            HBRUSH b = CreateSolidBrush(g_settingsAnimColor);
            FillRect(d->hDC, &d->rcItem, b); DeleteObject(b); return TRUE;
        }
        if (d && (d->CtlID == ID_SETTINGS_HOTKEY_SAVE || d->CtlID == ID_SETTINGS_HOTKEY_RESET)) {
            COLORREF bg = d->CtlID == g_settingsAnimButtonId ? g_settingsAnimColor :
                (temaAtual == 1 ? RGB(225,225,230) : temaAtual == 2 ? RGB(73,36,64) : temaAtual == 3 ? RGB(45,32,68) : RGB(55,55,62));
            HBRUSH br = CreateSolidBrush(bg); FillRect(d->hDC, &d->rcItem, br); DeleteObject(br);
            FrameRect(d->hDC, &d->rcItem, (HBRUSH)GetStockObject(GRAY_BRUSH));
            SetBkMode(d->hDC, TRANSPARENT);
            SetTextColor(d->hDC, temaAtual == 1 ? RGB(25,25,25) : RGB(245,245,250));
            wchar_t t[128] = {0}; GetWindowTextW(d->hwndItem, t, 127);
            DrawTextW(d->hDC, t, -1, &d->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &d->rcItem);
            return TRUE;
        }
    }
    if (msg == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == ID_SETTINGS_HOTKEY_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_LIST), LB_GETCURSEL, 0, 0);
            AtualizarListaAtalhos(hwnd, sel);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_APPLY) {
            int sel = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_LIST), LB_GETCURSEL, 0, 0);
            int ki = (int)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_KEY), CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_kodaShortcutCount && !g_kodaShortcuts[sel].fixed && ki >= 0) {
                UINT candidate = (UINT)SendMessageW(GetDlgItem(hwnd, ID_SETTINGS_HOTKEY_KEY), CB_GETITEMDATA, ki, 0);
                bool duplicate = candidate == VK_F11;
                for (int j = 0; j < g_kodaShortcutCount; ++j)
                    if (j != sel && !g_kodaShortcuts[j].fixed && g_kodaShortcuts[j].vk == candidate) duplicate = true;
                if (duplicate) {
                    MessageBoxW(hwnd, L"Essa tecla já está reservada ou em uso por outra função. Escolha outra tecla.", L"Atalho indisponível", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                g_kodaShortcuts[sel].vk = candidate;
                g_selectedShortcut = sel;
            }
            AtualizarListaAtalhos(hwnd, g_selectedShortcut);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_SAVE) {
            AnimarBotaoPreferencias(hwnd, id, RGB(45,190,95));
            SalvarConfiguracoesUsuario();
            MessageBoxW(hwnd, L"Todos os atalhos configuráveis foram salvos.", L"Kodanima", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (id == ID_SETTINGS_HOTKEY_RESET) {
            AnimarBotaoPreferencias(hwnd, id, RGB(220,55,65), true);
            for (int i = 0; i < g_kodaShortcutCount; ++i)
                if (!g_kodaShortcuts[i].fixed) g_kodaShortcuts[i].vk = g_kodaShortcuts[i].defaultVk;
            SalvarConfiguracoesUsuario();
            AtualizarListaAtalhos(hwnd, 0);
            return 0;
        }
    }
    if (msg == WM_CLOSE) {
        // O painel permanece aberto durante a edição; ele é encerrado com o workspace.
        ShowWindow(hwnd, SW_SHOW);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

#if KODA_GNU_X86_SIMD
// Calcula a cobertura de quatro pixels em paralelo. AVX (não AVX2) já permite
// vetores de double para a geometria do pincel; cada execução só ocorre depois
// de CPUID + OSXSAVE/XCR0 confirmar que o estado vetorial é seguro.
static __attribute__((target("avx"))) void KodaBlendarPincel4AVX(
    BYTE* pixels,int xStart,int y,int x1,int y1,double dx,double dy,double len2,
    double radius,double feather,double subpixelScale,int cr,int cg,int cb) {
    const __m256d zero=_mm256_setzero_pd(), one=_mm256_set1_pd(1.0);
    const __m256d xx=_mm256_set_pd((double)(xStart+3),(double)(xStart+2),(double)(xStart+1),(double)xStart);
    __m256d t=zero;
    if(len2>0.0001) {
        const __m256d proj=_mm256_add_pd(
            _mm256_mul_pd(_mm256_sub_pd(xx,_mm256_set1_pd((double)x1)),_mm256_set1_pd(dx)),
            _mm256_set1_pd(((double)y-y1)*dy));
        t=_mm256_max_pd(zero,_mm256_min_pd(one,_mm256_div_pd(proj,_mm256_set1_pd(len2))));
    }
    const __m256d qx=_mm256_add_pd(_mm256_set1_pd((double)x1),_mm256_mul_pd(t,_mm256_set1_pd(dx)));
    const __m256d qy=_mm256_add_pd(_mm256_set1_pd((double)y1),_mm256_mul_pd(t,_mm256_set1_pd(dy)));
    const __m256d vx=_mm256_sub_pd(xx,qx), vy=_mm256_sub_pd(_mm256_set1_pd((double)y),qy);
    const __m256d dist=_mm256_sqrt_pd(_mm256_add_pd(_mm256_mul_pd(vx,vx),_mm256_mul_pd(vy,vy)));
    __m256d coverage=_mm256_max_pd(zero,_mm256_min_pd(one,_mm256_div_pd(
        _mm256_sub_pd(_mm256_set1_pd(radius+feather*0.5),dist),_mm256_set1_pd(feather))));
    if(subpixelScale<1.0) coverage=_mm256_mul_pd(coverage,_mm256_set1_pd(subpixelScale));
    double cv[4]; _mm256_storeu_pd(cv,coverage);
    for(int i=0;i<4;++i) if(cv[i]>0.0) {
        const double inv=1.0-cv[i]; const size_t p=(size_t)i*4;
        pixels[p]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p]*inv+cb*cv[i])));
        pixels[p+1]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p+1]*inv+cg*cv[i])));
        pixels[p+2]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p+2]*inv+cr*cv[i])));
    }
}
static __attribute__((target("sse2"))) void KodaBlendarPincel2SSE2(
    BYTE* pixels,int xStart,int y,int x1,int y1,double dx,double dy,double len2,
    double radius,double feather,double subpixelScale,int cr,int cg,int cb) {
    const __m128d zero=_mm_setzero_pd(), one=_mm_set1_pd(1.0);
    const __m128d xx=_mm_set_pd((double)(xStart+1),(double)xStart);
    __m128d t=zero;
    if(len2>0.0001) {
        const __m128d proj=_mm_add_pd(
            _mm_mul_pd(_mm_sub_pd(xx,_mm_set1_pd((double)x1)),_mm_set1_pd(dx)),
            _mm_set1_pd(((double)y-y1)*dy));
        t=_mm_max_pd(zero,_mm_min_pd(one,_mm_div_pd(proj,_mm_set1_pd(len2))));
    }
    const __m128d qx=_mm_add_pd(_mm_set1_pd((double)x1),_mm_mul_pd(t,_mm_set1_pd(dx)));
    const __m128d qy=_mm_add_pd(_mm_set1_pd((double)y1),_mm_mul_pd(t,_mm_set1_pd(dy)));
    const __m128d vx=_mm_sub_pd(xx,qx), vy=_mm_sub_pd(_mm_set1_pd((double)y),qy);
    const __m128d dist=_mm_sqrt_pd(_mm_add_pd(_mm_mul_pd(vx,vx),_mm_mul_pd(vy,vy)));
    __m128d coverage=_mm_max_pd(zero,_mm_min_pd(one,_mm_div_pd(
        _mm_sub_pd(_mm_set1_pd(radius+feather*0.5),dist),_mm_set1_pd(feather))));
    if(subpixelScale<1.0) coverage=_mm_mul_pd(coverage,_mm_set1_pd(subpixelScale));
    double cv[2]; _mm_storeu_pd(cv,coverage);
    for(int i=0;i<2;++i) if(cv[i]>0.0) {
        const double inv=1.0-cv[i]; const size_t p=(size_t)i*4;
        pixels[p]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p]*inv+cb*cv[i])));
        pixels[p+1]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p+1]*inv+cg*cv[i])));
        pixels[p+2]=(BYTE)std::max(0,std::min(255,(int)std::lround(pixels[p+2]*inv+cr*cv[i])));
    }
}
#endif

static void DesenharLinhaAntiAliased(HDC dc, int x1, int y1, int x2, int y2, COLORREF cor, float largura, int dureza) {
    if (!dc) return;
    largura=std::max(0.01f,std::min(128.0f,largura));
    double dx=(double)x2-x1, dy=(double)y2-y1, len2=dx*dx+dy*dy;
    double radius=std::max(0.01,(double)largura*0.5);
    double feather=(3-std::max(0,std::min(3,dureza)))*0.65+0.35;
    int pad=(int)std::ceil(radius+feather+1.0);
    int left=std::min(x1,x2)-pad, right=std::max(x1,x2)+pad;
    int top=std::min(y1,y2)-pad, bottom=std::max(y1,y2)+pad;
    int cr=GetRValue(cor), cg=GetGValue(cor), cb=GetBValue(cor);
    // O bitmap da camada é uma HBITMAP selecionada no DC da pincelada. Limita a
    // área e faz uma única leitura/escrita em bloco, em vez de uma chamada GDI
    // para cada pixel (GetPixel/SetPixel eram o principal gargalo do anti-alias).
    HBITMAP target=(HBITMAP)GetCurrentObject(dc,OBJ_BITMAP);
    BITMAP targetInfo{};
    if(target && GetObjectW(target,sizeof(targetInfo),&targetInfo)==sizeof(targetInfo) &&
       targetInfo.bmWidth>0 && targetInfo.bmHeight>0 && targetInfo.bmWidthBytes>0) {
        left=std::max(0,left); top=std::max(0,top);
        right=std::min((int)targetInfo.bmWidth-1,right); bottom=std::min((int)targetInfo.bmHeight-1,bottom);
        int rw=right-left+1, rh=bottom-top+1;
        const unsigned long long area=(unsigned long long)(unsigned int)std::max(0,rw)*(unsigned long long)(unsigned int)std::max(0,rh);
        if(rw>0 && rh>0 && area<=16000000ULL && GarantirScratchPincel(rw,rh) &&
           BitBlt(g_brushBlendScratchDC,0,0,rw,rh,dc,left,top,SRCCOPY)) {
            BYTE* bits=g_brushBlendScratchBits; const size_t stride=(size_t)g_brushBlendScratchW*4;
            for(int yy=0;yy<rh;++yy) {
                const int y=top+yy; BYTE* row=bits+(size_t)yy*stride; int xx=0;
#if KODA_GNU_X86_SIMD
                if(g_kodaCPU.avxUsable) {
                    for(;xx+4<=rw;xx+=4)
                        KodaBlendarPincel4AVX(row+(size_t)xx*4,left+xx,y,x1,y1,dx,dy,len2,radius,feather,largura<1.0f?std::max(0.0,std::min(1.0,(double)largura)):1.0,cr,cg,cb);
                } else if(g_kodaCPU.sse2) {
                    for(;xx+2<=rw;xx+=2)
                        KodaBlendarPincel2SSE2(row+(size_t)xx*4,left+xx,y,x1,y1,dx,dy,len2,radius,feather,largura<1.0f?std::max(0.0,std::min(1.0,(double)largura)):1.0,cr,cg,cb);
                }
#endif
                for(;xx<rw;++xx) {
                    const int x=left+xx;
                    double t=0.0;
                    if(len2>0.0001) t=std::max(0.0,std::min(1.0,((x-x1)*dx+(y-y1)*dy)/len2));
                    double qx=x1+t*dx, qy=y1+t*dy;
                    double dist=std::sqrt((x-qx)*(x-qx)+(y-qy)*(y-qy));
                    double coverage=std::max(0.0,std::min(1.0,(radius+feather*0.5-dist)/feather));
                    if(largura<1.0f) coverage*=std::max(0.0,std::min(1.0,(double)largura));
                    if(coverage<=0.0) continue;
                    const size_t i=(size_t)xx*4;
                    int nb=(int)std::lround(row[i]*(1.0-coverage)+cb*coverage);
                    int ng=(int)std::lround(row[i+1]*(1.0-coverage)+cg*coverage);
                    int nr=(int)std::lround(row[i+2]*(1.0-coverage)+cr*coverage);
                    row[i]=(BYTE)std::max(0,std::min(255,nb));
                    row[i+1]=(BYTE)std::max(0,std::min(255,ng));
                    row[i+2]=(BYTE)std::max(0,std::min(255,nr));
                }
            }
            if(BitBlt(dc,left,top,rw,rh,g_brushBlendScratchDC,0,0,SRCCOPY)) return;
        }
    }
    // Fallback compatível caso o DC não tenha um bitmap endereçável ou a região
    // exceda o limite seguro do scratch.
    for(int y=top;y<=bottom;++y) for(int x=left;x<=right;++x) {
        double t=0.0;
        if(len2>0.0001) t=std::max(0.0,std::min(1.0,((x-x1)*dx+(y-y1)*dy)/len2));
        double qx=x1+t*dx, qy=y1+t*dy;
        double dist=std::sqrt((x-qx)*(x-qx)+(y-qy)*(y-qy));
        double coverage=std::max(0.0,std::min(1.0,(radius+feather*0.5-dist)/feather));
        if(largura<1.0f) coverage*=std::max(0.0,std::min(1.0,(double)largura));
        if(coverage<=0.0) continue;
        COLORREF old=GetPixel(dc,x,y); if(old==CLR_INVALID) continue;
        int nr=(int)std::lround(GetRValue(old)*(1.0-coverage)+cr*coverage);
        int ng=(int)std::lround(GetGValue(old)*(1.0-coverage)+cg*coverage);
        int nb=(int)std::lround(GetBValue(old)*(1.0-coverage)+cb*coverage);
        SetPixelV(dc,x,y,RGB(nr,ng,nb));
    }
}

// Armazenamento dos pincéis. A pasta-base vem do perfil do usuário atual:
// %APPDATA%\Pinceis do Kodanima. Não existe nome de usuário fixo no código.
static std::wstring TextoErroWin32Brush(DWORD code) {
    if(code==ERROR_SUCCESS) return L"sem código de erro adicional";
    LPWSTR buffer=NULL;
    DWORD n=FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,code,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPWSTR)&buffer,0,NULL);
    std::wstring msg;
    if(n && buffer) { msg.assign(buffer,n); LocalFree(buffer); }
    while(!msg.empty() && (msg.back()==L'\r'||msg.back()==L'\n'||msg.back()==L' ')) msg.pop_back();
    if(msg.empty()) { wchar_t num[48]; swprintf(num,48,L"código Win32 %lu",(unsigned long)code); msg=num; }
    return msg;
}
static void DefinirErroBrush(const wchar_t* etapa,const wchar_t* caminho,DWORD code) {
    g_brushStorageWin32Error=code;
    g_brushStorageError=(etapa?etapa:L"Falha no armazenamento dos pincéis");
    if(caminho && caminho[0]) { g_brushStorageError+=L"\nCaminho: "; g_brushStorageError+=caminho; }
    wchar_t num[48]; swprintf(num,48,L"\nErro Win32: %lu — ",(unsigned long)code);
    g_brushStorageError+=num; g_brushStorageError+=TextoErroWin32Brush(code);
}
static bool GarantirDiretorioKodanima(const wchar_t* path) {
    if(!path || !path[0]) { SetLastError(ERROR_INVALID_PARAMETER); return false; }

    DWORD attr=GetFileAttributesW(path);
    if(attr!=INVALID_FILE_ATTRIBUTES) {
        if(attr&FILE_ATTRIBUTE_DIRECTORY) return true;
        SetLastError(ERROR_DIRECTORY);
        return false;
    }

    // SHCreateDirectoryExW cria também diretórios intermediários ausentes.
    // Isso é importante para instalações novas e caminhos AppData redirecionados.
    int rc=SHCreateDirectoryExW(NULL,path,NULL);
    attr=GetFileAttributesW(path);
    if(attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError(ERROR_SUCCESS);
        return true;
    }

    DWORD err=(DWORD)rc;
    if(err==ERROR_SUCCESS) err=GetLastError();
    if(err==ERROR_SUCCESS || err==ERROR_ALREADY_EXISTS || err==ERROR_FILE_EXISTS)
        err=ERROR_DIRECTORY;
    SetLastError(err);
    return false;
}
static bool PastaGerenciadorBrushes(wchar_t* out, size_t cap) {
    if(!out || cap==0) { SetLastError(ERROR_INVALID_PARAMETER); return false; }
    out[0]=0; g_brushStorageError.clear(); g_brushStorageWin32Error=ERROR_SUCCESS;
    wchar_t roaming[MAX_PATH]={0};
    HRESULT hr=SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,roaming);
    if(FAILED(hr) || !roaming[0]) {
        DWORD n=GetEnvironmentVariableW(L"APPDATA",roaming,MAX_PATH);
        if(n==0 || n>=MAX_PATH) {
            DWORD e=GetLastError(); if(e==ERROR_SUCCESS) e=HRESULT_FACILITY(hr)==FACILITY_WIN32?HRESULT_CODE(hr):ERROR_PATH_NOT_FOUND;
            DefinirErroBrush(L"Não foi possível localizar a pasta Roaming do usuário atual",L"%APPDATA%",e);
            return false;
        }
    }
    std::wstring path=roaming;
    while(!path.empty() && (path.back()==L'\\' || path.back()==L'/')) path.pop_back();
    path+=L"\\Pinceis do Kodanima";
    if(path.size()+1>cap) { DefinirErroBrush(L"O caminho da biblioteca de pincéis excede o limite de buffer",path.c_str(),ERROR_INSUFFICIENT_BUFFER); return false; }
    lstrcpynW(out,path.c_str(),(int)cap);

    // Se a biblioteca já existe, apenas reutiliza o caminho; não recria nem apaga nada.
    DWORD attr=GetFileAttributesW(out);
    if(attr!=INVALID_FILE_ATTRIBUTES) {
        if(attr&FILE_ATTRIBUTE_DIRECTORY) return true;
        DefinirErroBrush(L"Já existe um arquivo com o nome reservado para a biblioteca de pincéis",out,ERROR_DIRECTORY);
        out[0]=0; return false;
    }
    // AppData\Roaming normalmente já existe. Criamos só a subpasta da biblioteca,
    // preservando o perfil do usuário e sem fixar o nome 'Absolute' no código.
    if(!CreateDirectoryW(out,NULL)) {
        DWORD e=GetLastError();
        // Se outra instância acabou de criá-la, basta reutilizá-la.
        attr=GetFileAttributesW(out);
        if(attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_DIRECTORY)) return true;
        if(e==ERROR_PATH_NOT_FOUND) {
            int rc=SHCreateDirectoryExW(NULL,out,NULL);
            attr=GetFileAttributesW(out);
            if(attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_DIRECTORY)) return true;
            if(rc>0) e=(DWORD)rc;
        }
        DefinirErroBrush(L"Não foi possível criar a pasta da biblioteca de pincéis no AppData\\Roaming",out,e);
        out[0]=0; return false;
    }
    return true;
}
static bool PastaTexturasBrushes(std::wstring& out) {
    wchar_t base[MAX_PATH]={0};
    if(!PastaGerenciadorBrushes(base,MAX_PATH)) return false;
    out=base; out+=L"\\texturas";
    if(!GarantirDiretorioKodanima(out.c_str())) {
        DWORD e=GetLastError(); if(e==ERROR_SUCCESS) e=ERROR_GEN_FAILURE;
        DefinirErroBrush(L"Não foi possível criar a subpasta de texturas",out.c_str(),e); return false;
    }
    return true;
}

// Leitor rasterizado sem dependência de gdiplus.lib: resolve GDI+ dinamicamente,
// disponível no Windows XP+, e converte BMP/PNG para uma máscara cinza do Kodanima.
// Transparência variável vira cobertura; imagens opacas usam luminância e a
// heurística de inversão para aceitar pontas claras ou escuras (incluindo MediBang/SAI).
struct KBrushGpImage;
typedef KBrushGpImage KBrushGpBitmap;
struct KBrushGpRect { INT X,Y,Width,Height; };
struct KBrushGpBitmapData { UINT Width,Height; INT Stride; INT PixelFormat; VOID* Scan0; UINT_PTR Reserved; };
struct KBrushGpStartupInput { UINT32 GdiplusVersion; void* DebugEventCallback; BOOL SuppressBackgroundThread; BOOL SuppressExternalCodecs; };
typedef INT (WINAPI *KBrushGdipStartupFn)(ULONG_PTR*,const KBrushGpStartupInput*,void*);
typedef VOID (WINAPI *KBrushGdipShutdownFn)(ULONG_PTR);
typedef INT (WINAPI *KBrushGdipCreateBitmapFn)(LPCWSTR,KBrushGpBitmap**);
typedef INT (WINAPI *KBrushGdipGetDimensionFn)(KBrushGpImage*,UINT*);
typedef INT (WINAPI *KBrushGdipLockBitsFn)(KBrushGpBitmap*,KBrushGpRect*,UINT,INT,KBrushGpBitmapData*);
typedef INT (WINAPI *KBrushGdipUnlockBitsFn)(KBrushGpBitmap*,KBrushGpBitmapData*);
typedef INT (WINAPI *KBrushGdipDisposeImageFn)(KBrushGpImage*);
static bool CarregarImagemComoMascara(const wchar_t* path,int& w,int& h,std::vector<unsigned char>& mask,std::wstring* errorOut=NULL) {
    w=h=0; mask.clear();
    HMODULE mod=LoadLibraryW(L"gdiplus.dll");
    if(!mod) { if(errorOut)*errorOut=L"O Windows não conseguiu carregar gdiplus.dll para ler BMP/PNG."; return false; }
    KBrushGdipStartupFn startup=(KBrushGdipStartupFn)GetProcAddress(mod,"GdiplusStartup");
    KBrushGdipShutdownFn shutdown=(KBrushGdipShutdownFn)GetProcAddress(mod,"GdiplusShutdown");
    KBrushGdipCreateBitmapFn create=(KBrushGdipCreateBitmapFn)GetProcAddress(mod,"GdipCreateBitmapFromFile");
    KBrushGdipGetDimensionFn getW=(KBrushGdipGetDimensionFn)GetProcAddress(mod,"GdipGetImageWidth");
    KBrushGdipGetDimensionFn getH=(KBrushGdipGetDimensionFn)GetProcAddress(mod,"GdipGetImageHeight");
    KBrushGdipLockBitsFn lockBits=(KBrushGdipLockBitsFn)GetProcAddress(mod,"GdipBitmapLockBits");
    KBrushGdipUnlockBitsFn unlockBits=(KBrushGdipUnlockBitsFn)GetProcAddress(mod,"GdipBitmapUnlockBits");
    KBrushGdipDisposeImageFn dispose=(KBrushGdipDisposeImageFn)GetProcAddress(mod,"GdipDisposeImage");
    if(!startup||!shutdown||!create||!getW||!getH||!lockBits||!unlockBits||!dispose) {
        if(errorOut)*errorOut=L"A versão do GDI+ não expõe todas as funções necessárias para ler a imagem."; FreeLibrary(mod); return false;
    }
    KBrushGpStartupInput si{}; si.GdiplusVersion=1; ULONG_PTR token=0;
    if(startup(&token,&si,NULL)!=0) { if(errorOut)*errorOut=L"GDI+ não iniciou o leitor rasterizado."; FreeLibrary(mod); return false; }
    KBrushGpBitmap* bitmap=NULL;
    INT st=create(path,&bitmap);
    if(st!=0 || !bitmap) {
        if(errorOut)*errorOut=L"Não foi possível decodificar a imagem. O arquivo deve ser um BMP ou PNG válido.";
        shutdown(token); FreeLibrary(mod); return false;
    }
    UINT iw=0,ih=0;
    if(getW((KBrushGpImage*)bitmap,&iw)!=0 || getH((KBrushGpImage*)bitmap,&ih)!=0 || iw<1 || ih<1 || iw>4096 || ih>4096) {
        if(errorOut)*errorOut=L"A imagem deve ter dimensões entre 1 e 4096 pixels em cada eixo.";
        dispose((KBrushGpImage*)bitmap); shutdown(token); FreeLibrary(mod); return false;
    }
    KBrushGpRect rect{0,0,(INT)iw,(INT)ih}; KBrushGpBitmapData data{};
    const INT PixelFormat32bppARGB=0x0026200A; const UINT ImageLockModeRead=1;
    st=lockBits(bitmap,&rect,ImageLockModeRead,PixelFormat32bppARGB,&data);
    if(st!=0 || !data.Scan0) {
        if(errorOut)*errorOut=L"GDI+ não conseguiu bloquear os pixels da imagem para leitura.";
        dispose((KBrushGpImage*)bitmap); shutdown(token); FreeLibrary(mod); return false;
    }
    bool alphaLow=false,alphaHigh=false; double borderSum=0.0; unsigned long long borderCount=0;
    for(unsigned int y=0;y<ih;++y) {
        const unsigned char* row=(const unsigned char*)data.Scan0+(ptrdiff_t)y*data.Stride;
        for(unsigned int x=0;x<iw;++x) {
            const unsigned char* px=row+(size_t)x*4;
            if(px[3]<250)alphaLow=true; if(px[3]>5)alphaHigh=true;
            // Amostra todo o contorno da imagem: isso é mais confiável que apenas
            // os quatro cantos quando a ponta encosta em um dos cantos.
            if(x==0 || y==0 || x+1==iw || y+1==ih) {
                borderSum+=(px[0]+px[1]+px[2])/3.0; ++borderCount;
            }
        }
    }
    const bool useAlpha=alphaLow&&alphaHigh;
    const double borderAvg=borderCount?borderSum/(double)borderCount:0.0;
    // Convenção interna: branco = cobertura do pincel; preto = fundo.
    // Em BMP totalmente opaco, se o contorno for claro, invertemos para extrair
    // a ponta escura sobre fundo branco; em fundo escuro, preservamos as partes claras.
    const bool invert=(!useAlpha && borderAvg>127.0);
    mask.resize((size_t)iw*(size_t)ih);
    for(unsigned int y=0;y<ih;++y) {
        const unsigned char* row=(const unsigned char*)data.Scan0+(ptrdiff_t)y*data.Stride;
        for(unsigned int x=0;x<iw;++x) {
            const unsigned char* px=row+(size_t)x*4; int value;
            if(useAlpha) value=px[3];
            else { double lum=px[0]*0.114+px[1]*0.587+px[2]*0.299; value=(int)std::lround(invert?255.0-lum:lum); }
            mask[(size_t)y*iw+x]=(unsigned char)std::max(0,std::min(255,value));
        }
    }
    unlockBits(bitmap,&data); dispose((KBrushGpImage*)bitmap); shutdown(token); FreeLibrary(mod);
    w=(int)iw; h=(int)ih; return true;
}
// Fallback nativo para BMPs já normalizados: não depende do leitor GDI+ e
// transforma a luminância em cobertura (preto=fundo, branco=pincel).
static bool CarregarMascaraBitmapGDI(HBITMAP hb,int& w,int& h,std::vector<unsigned char>& mask) {
    w=h=0; mask.clear(); if(!hb) return false; BITMAP bm{};
    if(!GetObjectW(hb,sizeof(bm),&bm) || bm.bmWidth<1 || bm.bmHeight<1 || bm.bmWidth>4096 || bm.bmHeight>4096) return false;
    BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=bm.bmWidth; bi.bmiHeader.biHeight=-bm.bmHeight; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
    std::vector<unsigned char> px((size_t)bm.bmWidth*(size_t)bm.bmHeight*4u); HDC dc=GetDC(NULL); if(!dc) return false;
    int lines=GetDIBits(dc,hb,0,(UINT)bm.bmHeight,px.data(),&bi,DIB_RGB_COLORS); ReleaseDC(NULL,dc);
    if(lines!=(int)bm.bmHeight) return false;
    mask.resize((size_t)bm.bmWidth*(size_t)bm.bmHeight);
    for(size_t i=0;i<mask.size();++i) { const unsigned char* q=&px[i*4u]; double lum=q[0]*0.114+q[1]*0.587+q[2]*0.299; mask[i]=(unsigned char)std::max(0,std::min(255,(int)std::lround(lum))); }
    w=bm.bmWidth; h=bm.bmHeight; return true;
}
static bool EscreverMascaraBMP(const wchar_t* path,int w,int h,const std::vector<unsigned char>& mask) {
    if(!path||w<1||h<1||w>4096||h>4096||mask.size()<(size_t)w*(size_t)h) { SetLastError(ERROR_INVALID_PARAMETER); return false; }
    const DWORD rowBytes=(DWORD)(((size_t)w*3u+3u)&~(size_t)3u);
    const ULONGLONG imageBytes=(ULONGLONG)rowBytes*(ULONGLONG)h;
    if(imageBytes>0xFFFFFFFFu-sizeof(BITMAPFILEHEADER)-sizeof(BITMAPINFOHEADER)) { SetLastError(ERROR_FILE_TOO_LARGE); return false; }
    BITMAPFILEHEADER fh{}; BITMAPINFOHEADER ih{};
    fh.bfType=0x4D42; fh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER); fh.bfSize=fh.bfOffBits+(DWORD)imageBytes;
    ih.biSize=sizeof(BITMAPINFOHEADER); ih.biWidth=w; ih.biHeight=h; ih.biPlanes=1; ih.biBitCount=24; ih.biCompression=BI_RGB; ih.biSizeImage=(DWORD)imageBytes; ih.biXPelsPerMeter=ih.biYPelsPerMeter=3780;
    HANDLE f=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) return false;
    auto put=[&](const void* pData,DWORD cb)->bool {
        DWORD n=0;
        if(!WriteFile(f,pData,cb,&n,NULL)) return false;
        if(n!=cb) { SetLastError(ERROR_WRITE_FAULT); return false; }
        return true;
    };
    bool ok=put(&fh,sizeof(fh))&&put(&ih,sizeof(ih));
    std::vector<unsigned char> row(rowBytes,0);
    for(int y=h-1;ok&&y>=0;--y) {
        for(int x=0;x<w;++x) { unsigned char v=mask[(size_t)y*w+x]; row[(size_t)x*3]=v; row[(size_t)x*3+1]=v; row[(size_t)x*3+2]=v; }
        ok=put(row.data(),rowBytes);
    }
    DWORD err=ok?ERROR_SUCCESS:GetLastError(); CloseHandle(f);
    if(!ok) { DeleteFileW(path); SetLastError(err?err:ERROR_WRITE_FAULT); return false; }
    return true;
}
static std::wstring NomeArquivoSeguroBrush(std::wstring name) {
    for(size_t i=0;i<name.size();++i) if(name[i]<32 || name[i]==L'\\'||name[i]==L'/'||name[i]==L':'||name[i]==L'*'||name[i]==L'?'||name[i]==L'"'||name[i]==L'<'||name[i]==L'>'||name[i]==L'|') name[i]=L'_';
    while(!name.empty()&&(name.back()==L' '||name.back()==L'.')) name.pop_back();
    while(!name.empty()&&name.front()==L' ') name.erase(name.begin());
    if(name.empty()) name=L"Pincel";
    if(name.size()>90) name.resize(90);
    return name;
}
static bool ArquivoExisteBrush(const std::wstring& path);
static std::wstring CaminhoJsonBibliotecaBrushes() {
    wchar_t folder[MAX_PATH]={0};
    if(!PastaGerenciadorBrushes(folder,MAX_PATH)) return std::wstring();
    std::wstring p=folder; p+=L"\\brushes.json"; return p;
}
static std::wstring EscaparJsonBrush(const std::wstring& src) {
    std::wstring out; out.reserve(src.size()+8);
    for(size_t i=0;i<src.size();++i) {
        wchar_t c=src[i];
        if(c==L'\\') out+=L"\\\\";
        else if(c==L'"') out+=L"\\\"";
        else if(c==L'\n') out+=L"\\n";
        else if(c==L'\r') out+=L"\\r";
        else if(c==L'\t') out+=L"\\t";
        else if(c>=32) out.push_back(c);
    }
    return out;
}
static bool JsonLerStringBrush(const std::wstring& obj,const wchar_t* key,std::wstring& out) {
    out.clear(); std::wstring token=L"\""; token+=key; token+=L"\"";
    size_t p=obj.find(token); if(p==std::wstring::npos) return false;
    p=obj.find(L':',p+token.size()); if(p==std::wstring::npos) return false; ++p;
    while(p<obj.size() && (obj[p]==L' '||obj[p]==L'\t'||obj[p]==L'\r'||obj[p]==L'\n')) ++p;
    if(p>=obj.size()||obj[p]!=L'\"') return false; ++p;
    while(p<obj.size()) {
        wchar_t c=obj[p++];
        if(c==L'\"') return true;
        if(c==L'\\' && p<obj.size()) {
            wchar_t e=obj[p++];
            if(e==L'\\'||e==L'\"'||e==L'/') out.push_back(e);
            else if(e==L'n') out.push_back(L'\n');
            else if(e==L'r') out.push_back(L'\r');
            else if(e==L't') out.push_back(L'\t');
            else if(e==L'b') out.push_back(L'\b');
            else if(e==L'f') out.push_back(L'\f');
            else out.push_back(e);
        } else out.push_back(c);
    }
    return false;
}
static bool JsonLerNumeroBrush(const std::wstring& obj,const wchar_t* key,double& out) {
    std::wstring token=L"\""; token+=key; token+=L"\"";
    size_t p=obj.find(token); if(p==std::wstring::npos) return false;
    p=obj.find(L':',p+token.size()); if(p==std::wstring::npos) return false; ++p;
    while(p<obj.size() && (obj[p]==L' '||obj[p]==L'\t'||obj[p]==L'\r'||obj[p]==L'\n')) ++p;
    if(p>=obj.size()) return false;
    wchar_t* end=NULL; double v=wcstod(obj.c_str()+p,&end); if(end==obj.c_str()+p) return false;
    out=v; return true;
}
static bool JsonLerBoolBrush(const std::wstring& obj,const wchar_t* key,bool& out) {
    std::wstring token=L"\""; token+=key; token+=L"\"";
    size_t p=obj.find(token); if(p==std::wstring::npos) return false;
    p=obj.find(L':',p+token.size()); if(p==std::wstring::npos) return false; ++p;
    while(p<obj.size() && (obj[p]==L' '||obj[p]==L'\t'||obj[p]==L'\r'||obj[p]==L'\n')) ++p;
    if(p>=obj.size()) return false;
    if(obj.compare(p,4,L"true")==0) { out=true; return true; }
    if(obj.compare(p,5,L"false")==0) { out=false; return true; }
    double n=0.0; if(JsonLerNumeroBrush(obj,key,n)){out=n!=0.0;return true;} return false;
}
static bool ConverterUtf8ParaWideBrush(const char* data,size_t bytes,std::wstring& out) {
    out.clear(); if(!data || bytes==0) return true;
    size_t off=0;
    if(bytes>=3 && (unsigned char)data[0]==0xEF && (unsigned char)data[1]==0xBB && (unsigned char)data[2]==0xBF) off=3;
    if(bytes>=2 && (unsigned char)data[0]==0xFF && (unsigned char)data[1]==0xFE) {
        size_t n=(bytes-2)/sizeof(wchar_t); out.assign(n,L'\0'); if(n) memcpy(&out[0],data+2,n*sizeof(wchar_t)); return true;
    }
    int n=MultiByteToWideChar(CP_UTF8,0,data+off,(int)(bytes-off),NULL,0);
    if(n<=0) { n=MultiByteToWideChar(CP_ACP,0,data+off,(int)(bytes-off),NULL,0); if(n<=0)return false; out.assign((size_t)n,L'\0'); return MultiByteToWideChar(CP_ACP,0,data+off,(int)(bytes-off),&out[0],n)>0; }
    out.assign((size_t)n,L'\0'); return MultiByteToWideChar(CP_UTF8,0,data+off,(int)(bytes-off),&out[0],n)>0;
}
static bool EscreverTextoUtf8Brush(const std::wstring& path,const std::wstring& text) {
    if(text.size()>(size_t)INT_MAX/4) { SetLastError(ERROR_FILE_TOO_LARGE); return false; }
    int cb=WideCharToMultiByte(CP_UTF8,0,text.c_str(),(int)text.size(),NULL,0,NULL,NULL);
    if(cb<0 || (cb==0 && !text.empty())) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return false; }
    std::vector<char> bytes((size_t)cb);
    if(cb && WideCharToMultiByte(CP_UTF8,0,text.c_str(),(int)text.size(),bytes.data(),cb,NULL,NULL)<=0) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return false; }
    HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) return false;
    size_t pos=0; bool ok=true;
    while(pos<bytes.size()) { DWORD chunk=(DWORD)std::min((size_t)0x7FFFFFFFu,bytes.size()-pos), wrote=0; if(!WriteFile(f,bytes.data()+pos,chunk,&wrote,NULL)||wrote!=chunk) { if(GetLastError()==ERROR_SUCCESS)SetLastError(ERROR_WRITE_FAULT); ok=false; break; } pos+=wrote; }
    DWORD err=ok?ERROR_SUCCESS:GetLastError(); if(!CloseHandle(f)&&ok){err=GetLastError();ok=false;} if(!ok)SetLastError(err?err:ERROR_WRITE_FAULT); else SetLastError(ERROR_SUCCESS); return ok;
}
static bool LerDocumentoJsonBrush(std::wstring& doc) {
    doc.clear(); std::wstring path=CaminhoJsonBibliotecaBrushes(); if(path.empty()) return false;
    HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{}; if(!GetFileSizeEx(f,&size)||size.QuadPart<=0||size.QuadPart>16LL*1024*1024) { CloseHandle(f); return false; }
    std::vector<char> bytes((size_t)size.QuadPart); DWORD got=0; BOOL ok=ReadFile(f,bytes.data(),(DWORD)bytes.size(),&got,NULL); CloseHandle(f);
    if(!ok || got!=bytes.size()) return false;
    return ConverterUtf8ParaWideBrush(bytes.data(),bytes.size(),doc) && !doc.empty();
}
static bool JsonExtrairObjetoBrush(const std::wstring& doc,size_t& pos,std::wstring& obj) {
    size_t start=doc.find(L'{',pos); if(start==std::wstring::npos) return false;
    bool quoted=false,escaped=false; int depth=0;
    for(size_t i=start;i<doc.size();++i) {
        wchar_t c=doc[i];
        if(quoted) { if(escaped) escaped=false; else if(c==L'\\') escaped=true; else if(c==L'\"') quoted=false; continue; }
        if(c==L'\"') { quoted=true; continue; }
        if(c==L'{') ++depth;
        else if(c==L'}') { --depth; if(depth==0) { obj=doc.substr(start,i-start+1); pos=i+1; return true; } }
    }
    return false;
}
static bool LerMetaObjetoJsonBrush(const std::wstring& obj,BrushMetaKodanima& m,std::wstring& texture) {
    m=MetaPadraoImportacao(); texture.clear(); double n=0.0; bool b=false;
    if(JsonLerNumeroBrush(obj,L"size",n)) m.size=(float)std::max(0.01,std::min(20000.0,n));
    if(JsonLerNumeroBrush(obj,L"minSize",n)) m.minSize=(float)std::max(0.01,std::min(5000.0,n));
    if(JsonLerNumeroBrush(obj,L"maxSize",n)) m.maxSize=(float)std::max((double)m.minSize,std::min(5000.0,n));
    if(JsonLerNumeroBrush(obj,L"hardness",n)) m.hardness=(int)std::max(0.0,std::min(3.0,n));
    if(JsonLerBoolBrush(obj,L"antialias",b)) m.antialias=b?1:0;
    if(JsonLerNumeroBrush(obj,L"spacingPercent",n)) m.spacingPercent=(float)std::max(1.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"angleDegrees",n)) m.angleDegrees=(float)std::max(-180.0,std::min(180.0,n));
    if(JsonLerNumeroBrush(obj,L"roundnessPercent",n)) m.roundnessPercent=(float)std::max(1.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"scatterPercent",n)) m.scatterPercent=(float)std::max(0.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"opacityPercent",n)) m.opacityPercent=(float)std::max(0.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"flowPercent",n)) m.flowPercent=(float)std::max(1.0,std::min(100.0,n));
    if(JsonLerBoolBrush(obj,L"rotateWithStroke",b)) m.rotateWithStroke=b?1:0;
    if(JsonLerBoolBrush(obj,L"sizeByPressure",b)) m.sizeByPressure=b?1:0;
    if(JsonLerBoolBrush(obj,L"opacityByPressure",b)) m.opacityByPressure=b?1:0;
    if(JsonLerNumeroBrush(obj,L"startSizePercent",n)) m.startSizePercent=(float)std::max(1.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"endSizePercent",n)) m.endSizePercent=(float)std::max(1.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"startOpacityPercent",n)) m.startOpacityPercent=(float)std::max(0.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"endOpacityPercent",n)) m.endOpacityPercent=(float)std::max(0.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"sprayDensity",n)) m.sprayDensity=(float)std::max(1.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"sprayRadiusPercent",n)) m.sprayRadiusPercent=(float)std::max(0.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"particleCount",n)) m.particleCount=(int)std::max(1.0,std::min(32.0,n));
    if(JsonLerNumeroBrush(obj,L"particleSpreadPercent",n)) m.particleSpreadPercent=(float)std::max(0.0,std::min(500.0,n));
    if(JsonLerNumeroBrush(obj,L"scaleRandomPercent",n)) m.scaleRandomPercent=(float)std::max(0.0,std::min(100.0,n));
    if(JsonLerNumeroBrush(obj,L"blurPercent",n)) m.blurPercent=(float)std::max(0.0,std::min(100.0,n));
    if(JsonLerBoolBrush(obj,L"sprayingEffect",b)) m.sprayingEffect=b?1:0;
    if(JsonLerBoolBrush(obj,L"randomRotation",b)) m.randomRotation=b?1:0;
    if(JsonLerBoolBrush(obj,L"followPenDirection",b)) m.followPenDirection=b?1:0;
    if(JsonLerBoolBrush(obj,L"antiOverflow",b)) m.antiOverflow=b?1:0;
    if(JsonLerBoolBrush(obj,L"optimizeStroke",b)) m.optimizeStroke=b?1:0;
    if(JsonLerBoolBrush(obj,L"startEndDynamics",b)) m.startEndDynamics=b?1:0;
    if(JsonLerBoolBrush(obj,L"particlesEnabled",b)) m.particlesEnabled=b?1:0;
    JsonLerStringBrush(obj,L"texture",texture);
    return true;
}
static bool SalvarBibliotecaBrushesJson(const std::vector<BrushEntryKodanima>& entries) {
    std::wstring path=CaminhoJsonBibliotecaBrushes();
    if(path.empty()) return false;
    std::wstring tmp=path+L".tmp";
    SetLastError(ERROR_SUCCESS);
    std::wostringstream f; f.imbue(std::locale::classic());
    f << L"{\n  \"format\": \"KodanimaBrushLibrary\",\n  \"version\": 1,\n  \"useImportedBrushes\": " << (g_useImportedBrushes?L"true":L"false") << L",\n  \"brushes\": [\n";
    for(size_t i=0;i<entries.size();++i) {
        const BrushEntryKodanima& e=entries[i]; const BrushMetaKodanima& m=e.meta;
        const wchar_t* base=wcsrchr(e.bmpPath.c_str(),L'\\'); base=base?base+1:e.bmpPath.c_str();
        if(i) f << L",\n";
        f << L"    {\n      \"name\": \"" << EscaparJsonBrush(e.name) << L"\",\n"
          << L"      \"bmp\": \"" << EscaparJsonBrush(base) << L"\",\n"
          << L"      \"texture\": \"" << EscaparJsonBrush(e.texturePath) << L"\",\n"
          << L"      \"size\": " << m.size << L",\n"
          << L"      \"minSize\": " << m.minSize << L",\n"
          << L"      \"maxSize\": " << m.maxSize << L",\n"
          << L"      \"hardness\": " << m.hardness << L",\n"
          << L"      \"antialias\": " << (m.antialias?L"true":L"false") << L",\n"
          << L"      \"spacingPercent\": " << m.spacingPercent << L",\n"
          << L"      \"angleDegrees\": " << m.angleDegrees << L",\n"
          << L"      \"roundnessPercent\": " << m.roundnessPercent << L",\n"
          << L"      \"scatterPercent\": " << m.scatterPercent << L",\n"
          << L"      \"opacityPercent\": " << m.opacityPercent << L",\n"
          << L"      \"flowPercent\": " << m.flowPercent << L",\n"
          << L"      \"rotateWithStroke\": " << (m.rotateWithStroke?L"true":L"false") << L",\n"
          << L"      \"sizeByPressure\": " << (m.sizeByPressure?L"true":L"false") << L",\n"
          << L"      \"opacityByPressure\": " << (m.opacityByPressure?L"true":L"false") << L",\n"
          << L"      \"startSizePercent\": " << m.startSizePercent << L",\n"
          << L"      \"endSizePercent\": " << m.endSizePercent << L",\n"
          << L"      \"startOpacityPercent\": " << m.startOpacityPercent << L",\n"
          << L"      \"endOpacityPercent\": " << m.endOpacityPercent << L",\n"
          << L"      \"sprayDensity\": " << m.sprayDensity << L",\n"
          << L"      \"sprayRadiusPercent\": " << m.sprayRadiusPercent << L",\n"
          << L"      \"particleCount\": " << m.particleCount << L",\n"
          << L"      \"particleSpreadPercent\": " << m.particleSpreadPercent << L",\n"
          << L"      \"scaleRandomPercent\": " << m.scaleRandomPercent << L",\n"
          << L"      \"blurPercent\": " << m.blurPercent << L",\n"
          << L"      \"sprayingEffect\": " << (m.sprayingEffect?L"true":L"false") << L",\n"
          << L"      \"randomRotation\": " << (m.randomRotation?L"true":L"false") << L",\n"
          << L"      \"followPenDirection\": " << (m.followPenDirection?L"true":L"false") << L",\n"
          << L"      \"antiOverflow\": " << (m.antiOverflow?L"true":L"false") << L",\n"
          << L"      \"optimizeStroke\": " << (m.optimizeStroke?L"true":L"false") << L",\n"
          << L"      \"startEndDynamics\": " << (m.startEndDynamics?L"true":L"false") << L",\n"
          << L"      \"particlesEnabled\": " << (m.particlesEnabled?L"true":L"false") << L",\n"
          << L"      \"texture2\": \"" << EscaparJsonBrush(e.texture2Path) << L"\",\n"
          << L"      \"texture3\": \"" << EscaparJsonBrush(e.texture3Path) << L"\",\n"
          << L"      \"texture2Tint\": " << (unsigned long)e.texture2Tint << L",\n"
          << L"      \"texture3Tint\": " << (unsigned long)e.texture3Tint << L",\n"
          << L"      \"texture2Opacity\": " << e.texture2Opacity << L",\n"
          << L"      \"texture3Opacity\": " << e.texture3Opacity << L"\n    }";
    }
    f << L"\n  ]\n}\n";
    const std::wstring document=f.str();
    if(!EscreverTextoUtf8Brush(tmp,document)) { DWORD er=GetLastError(); if(er==ERROR_SUCCESS)er=ERROR_WRITE_FAULT; DefinirErroBrush(L"Falha ao escrever todos os parâmetros no JSON mestre",tmp.c_str(),er); DeleteFileW(tmp.c_str()); return false; }
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        DWORD er=GetLastError(); DefinirErroBrush(L"O JSON foi escrito, mas o Windows não conseguiu atualizar o arquivo mestre",path.c_str(),er); DeleteFileW(tmp.c_str()); return false;
    }
    return true;
}
static bool LerRegistroBrushJson(const std::wstring& bmpName,BrushMetaKodanima& meta,std::wstring& texture) {
    std::wstring doc; if(!LerDocumentoJsonBrush(doc)) return false;
    size_t array=doc.find(L"\"brushes\""); if(array==std::wstring::npos) return false;
    size_t pos=array; std::wstring obj;
    while(JsonExtrairObjetoBrush(doc,pos,obj)) {
        std::wstring bmp;
        if(JsonLerStringBrush(obj,L"bmp",bmp) && _wcsicmp(bmp.c_str(),bmpName.c_str())==0)
            return LerMetaObjetoJsonBrush(obj,meta,texture);
        size_t next=doc.find(L'{',pos); if(next==std::wstring::npos) break; pos=next;
    }
    return false;
}
static bool AtualizarRegistroBrushJson(const BrushEntryKodanima& entry,const BrushMetaKodanima* preset,const std::wstring* texturePath=NULL) {
    std::vector<BrushEntryKodanima> entries=g_brushesKodanima;
    const std::wstring fullPath=entry.bmpPath;
    size_t found=entries.size();
    for(size_t i=0;i<entries.size();++i) if(_wcsicmp(entries[i].bmpPath.c_str(),fullPath.c_str())==0) { found=i; break; }
    BrushEntryKodanima value=entry;
    if(found<entries.size()) value=entries[found];
    // Preserve alterações editáveis do registro (nome e camadas de textura) ao atualizar metadados.
    value.name=entry.name; value.texture2Path=entry.texture2Path; value.texture3Path=entry.texture3Path;
    value.texture2Tint=entry.texture2Tint; value.texture3Tint=entry.texture3Tint;
    value.texture2Opacity=entry.texture2Opacity; value.texture3Opacity=entry.texture3Opacity;
    if(preset) value.meta=*preset;
    if(texturePath) value.texturePath=*texturePath;
    if(found==entries.size()) entries.push_back(value); else entries[found]=value;
    if(!SalvarBibliotecaBrushesJson(entries)) return false;
    g_brushesKodanima.swap(entries);
    return true;
}
static bool SalvarAssociacaoTextura(const BrushEntryKodanima& e,const std::wstring& path) {
    if(!AtualizarRegistroBrushJson(e,NULL,&path)) {
        if(g_brushStorageError.empty()) DefinirErroBrush(L"Não foi possível salvar o vínculo da textura no JSON mestre",e.bmpPath.c_str(),ERROR_WRITE_FAULT);
        return false;
    }
    return true;
}
static bool LerAssociacaoTextura(const BrushEntryKodanima& e,std::wstring& path) {
    path=e.texturePath; return !path.empty();
}
static void AtivarBrushSelecionado(int index) {
    g_brushSelecionado=index;
    g_activeBrushTextureMask.clear(); g_activeBrushTextureWidth=g_activeBrushTextureHeight=0; g_activeBrushTexturePath.clear();
    if(g_activeBrushBitmap){DeleteObject(g_activeBrushBitmap);g_activeBrushBitmap=NULL;}
    g_activeBrushMask.clear(); g_activeBrushWidth=g_activeBrushHeight=0; g_activeBrushPath.clear();
    if(index<0||index>=(int)g_brushesKodanima.size()) return;
    const BrushEntryKodanima& e=g_brushesKodanima[index]; g_activeBrushPath=e.bmpPath;
    g_activeBrushBitmap=(HBITMAP)LoadImageW(NULL,e.bmpPath.c_str(),IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    // Carrega a máscara pela mesma rotina usada na importação. Assim, o bitmap
    // normalizado não passa por uma segunda heurística invertida na ativação.
    if(g_activeBrushBitmap) {
        std::wstring loadError;
        if(!CarregarImagemComoMascara(e.bmpPath.c_str(),g_activeBrushWidth,g_activeBrushHeight,g_activeBrushMask,&loadError)) {
            g_activeBrushMask.clear(); g_activeBrushWidth=g_activeBrushHeight=0;
            CarregarMascaraBitmapGDI(g_activeBrushBitmap,g_activeBrushWidth,g_activeBrushHeight,g_activeBrushMask);
        }
        // Último recurso: tenta ler por GDI mesmo se LoadImage retornou um bitmap
        // que o GDI+ não conseguiu decodificar corretamente.
        if(g_activeBrushMask.empty()) CarregarMascaraBitmapGDI(g_activeBrushBitmap,g_activeBrushWidth,g_activeBrushHeight,g_activeBrushMask);
    }
    BrushMetaKodanima m=e.meta;
    tamanhoLapis=std::max(0.01f,std::min(20000.0f,m.size));
    tamanhoMinimoPressao=std::max(0.01f,std::min(5000.0f,m.minSize));
    tamanhoMaximoPressao=std::max(tamanhoMinimoPressao,std::min(5000.0f,m.maxSize));
    durezaPincel=std::max(0,std::min(3,m.hardness)); lapisAntiAlias=m.antialias!=0;
    brushSpacingPercent=std::max(1.0f,std::min(500.0f,m.spacingPercent));
    brushAngleDegrees=std::max(-180.0f,std::min(180.0f,m.angleDegrees));
    brushRoundnessPercent=std::max(1.0f,std::min(100.0f,m.roundnessPercent));
    brushScatterPercent=std::max(0.0f,std::min(500.0f,m.scatterPercent));
    brushOpacityPercent=std::max(0.0f,std::min(100.0f,m.opacityPercent));
    brushFlowPercent=std::max(1.0f,std::min(100.0f,m.flowPercent));
    brushRotateWithStroke=(m.rotateWithStroke!=0)&&(m.followPenDirection!=0); brushSizeByPressure=m.sizeByPressure!=0; brushOpacityByPressure=m.opacityByPressure!=0;
    g_brushOptimizedStroke=m.optimizeStroke!=0;
    g_activeBrushTexture2Path=e.texture2Path; g_activeBrushTexture3Path=e.texture3Path; g_activeBrushTexture2Tint=e.texture2Tint; g_activeBrushTexture3Tint=e.texture3Tint; g_activeBrushTexture2Opacity=e.texture2Opacity; g_activeBrushTexture3Opacity=e.texture3Opacity;
    g_activeBrushTextureMask2.clear(); g_activeBrushTextureMask3.clear(); g_activeBrushTexture2Width=g_activeBrushTexture2Height=g_activeBrushTexture3Width=g_activeBrushTexture3Height=0;
    if(!e.texture2Path.empty()) { std::wstring why; CarregarImagemComoMascara(e.texture2Path.c_str(),g_activeBrushTexture2Width,g_activeBrushTexture2Height,g_activeBrushTextureMask2,&why); }
    if(!e.texture3Path.empty()) { std::wstring why; CarregarImagemComoMascara(e.texture3Path.c_str(),g_activeBrushTexture3Width,g_activeBrushTexture3Height,g_activeBrushTextureMask3,&why); }
    SalvarConfiguracoesUsuario();
    std::wstring texturePath;
    if(LerAssociacaoTextura(e,texturePath)) {
        std::wstring why; int tw=0,th=0; std::vector<unsigned char> tm;
        if(CarregarImagemComoMascara(texturePath.c_str(),tw,th,tm,&why)) {
            g_activeBrushTexturePath=texturePath; g_activeBrushTextureWidth=tw; g_activeBrushTextureHeight=th; g_activeBrushTextureMask.swap(tm);
        }
    }
    if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
}
static void DesenharLinhaComPontaBitmap(HDC dc,int x1,int y1,int x2,int y2,COLORREF cor,float largura) {
    if(!dc||!g_activeBrushBitmap||g_activeBrushMask.empty()||g_activeBrushWidth<1||g_activeBrushHeight<1){DesenharLinhaAntiAliased(dc,x1,y1,x2,y2,cor,largura,durezaPincel);return;}
    const BrushMetaKodanima* activeMeta=(g_brushSelecionado>=0&&g_brushSelecionado<(int)g_brushesKodanima.size())?&g_brushesKodanima[g_brushSelecionado].meta:NULL;
    const double blurAmount=activeMeta?std::max(0.0,std::min(1.0,(double)activeMeta->blurPercent/100.0)):0.0;
    const int dw=std::max(1,std::min(512,(int)std::lround(largura)));
    const double roundness=std::max(0.01,std::min(1.0,(double)brushRoundnessPercent/100.0));
    const int dh=std::max(1,std::min(512,(int)std::lround((double)dw*g_activeBrushHeight/std::max(1,g_activeBrushWidth)*roundness)));
    const double dx=(double)x2-x1,dy=(double)y2-y1,len=std::sqrt(dx*dx+dy*dy);
    const double spacing=std::max(1.0,(double)dw*std::max(1.0f,std::min(500.0f,brushSpacingPercent))/100.0);
    const int steps=std::max(1,(int)std::ceil(len/spacing));
    const double baseAngle=brushAngleDegrees*3.14159265358979323846/180.0;
    const double pathAngle=(len>0.001&&brushRotateWithStroke)?std::atan2(dy,dx):0.0;
    const int cr=GetRValue(cor),cg=GetGValue(cor),cb=GetBValue(cor);
    double pressure=std::max(0.0,std::min(1.0,(double)pressaoAtualCaneta));
    double opacity=std::max(0.0,std::min(1.0,(double)brushOpacityPercent/100.0))*std::max(0.01,std::min(1.0,(double)brushFlowPercent/100.0));
    // Um carimbo de bitmap não pode ter largura inteira menor que 1 px; para respeitar
    // a espessura mínima fracionária, reduzimos a cobertura/alpha abaixo de 1 pixel.
    if(largura<1.0f) opacity*=std::max(0.0,std::min(1.0,(double)largura));
    if(brushOpacityByPressure) opacity*=pressure;
    RECT clipRect{}; const bool hasClip=GetClipBox(dc,&clipRect)!=ERROR;
    const bool antiOverflow=(g_brushesKodanima.size()>0 && g_brushSelecionado>=0 && g_brushSelecionado<(int)g_brushesKodanima.size() && g_brushesKodanima[g_brushSelecionado].meta.antiOverflow!=0);
    // Cada carimbo calcula seu próprio ângulo e dimensões; sem alocação por pixel.
    for(int st=0;st<=steps;++st){
        const double t=(double)st/steps;
        int baseCx=(int)std::lround(x1+dx*t),baseCy=(int)std::lround(y1+dy*t);
        const int copies=(g_brushesKodanima.size()>0 && g_brushSelecionado>=0 && g_brushSelecionado<(int)g_brushesKodanima.size()) ? (g_brushesKodanima[g_brushSelecionado].meta.sprayingEffect ? std::max(1,std::min(32,(int)std::lround(g_brushesKodanima[g_brushSelecionado].meta.sprayDensity/3.0f))) : (g_brushesKodanima[g_brushSelecionado].meta.particlesEnabled ? std::max(1,std::min(32,g_brushesKodanima[g_brushSelecionado].meta.particleCount)) : 1)) : 1;
        for(int particle=0;particle<copies;++particle){
        int cx=baseCx,cy=baseCy;
        int stampW=dw, stampH=dh;
        if(g_brushesKodanima.size()>0 && g_brushSelecionado>=0 && g_brushSelecionado<(int)g_brushesKodanima.size()) { const BrushMetaKodanima& mm=g_brushesKodanima[g_brushSelecionado].meta; if(mm.startEndDynamics){double factor=(double)mm.startSizePercent+(double)(mm.endSizePercent-mm.startSizePercent)*t;stampW=std::max(1,std::min(512,(int)std::lround(dw*factor/100.0)));stampH=std::max(1,std::min(512,(int)std::lround(dh*factor/100.0)));} }
        if(activeMeta && activeMeta->scaleRandomPercent>0.0f) { unsigned int scaleSeed=(unsigned int)(baseCx*2654435761u)^(unsigned int)(baseCy*2246822519u)^(unsigned int)((st+11)*(particle+19)*3266489917u); scaleSeed=scaleSeed*1664525u+1013904223u; double unit=((scaleSeed&65535u)/32767.5)-1.0; double sf=std::max(0.10,1.0+unit*std::min(100.0,(double)activeMeta->scaleRandomPercent)/100.0); stampW=std::max(1,std::min(512,(int)std::lround(stampW*sf))); stampH=std::max(1,std::min(512,(int)std::lround(stampH*sf))); }
        double stampAngle=baseAngle+pathAngle;
        if(g_brushesKodanima.size()>0 && g_brushSelecionado>=0 && g_brushSelecionado<(int)g_brushesKodanima.size() && g_brushesKodanima[g_brushSelecionado].meta.randomRotation){unsigned int rs=(unsigned int)(baseCx*2246822519u)^(unsigned int)(baseCy*3266489917u)^(unsigned int)((st+1)*(particle+31)*668265263u);rs=rs*1664525u+1013904223u;stampAngle+=(((rs&65535u)/65535.0)*2.0-1.0)*3.14159265358979323846;}
        const double cs=std::cos(stampAngle),sn=std::sin(stampAngle);
        const int halfX=(int)std::ceil(std::fabs(cs)*stampW/2.0+std::fabs(sn)*stampH/2.0)+1;
        const int halfY=(int)std::ceil(std::fabs(sn)*stampW/2.0+std::fabs(cs)*stampH/2.0)+1;
        if(particle>0){ unsigned int ps=(unsigned int)(baseCx*73856093u)^(unsigned int)(baseCy*19349663u)^(unsigned int)((st+1)*(particle+17)*83492791u); ps=ps*1664525u+1013904223u; double spread=(g_brushesKodanima[g_brushSelecionado].meta.sprayingEffect?g_brushesKodanima[g_brushSelecionado].meta.sprayRadiusPercent:g_brushesKodanima[g_brushSelecionado].meta.particleSpreadPercent); double rr=((ps&65535u)/65535.0)*dw*std::max(0.0,std::min(500.0,spread))/100.0; ps=ps*1664525u+1013904223u; double aa=((ps&65535u)/65535.0)*6.283185307179586; cx+=(int)std::lround(std::cos(aa)*rr); cy+=(int)std::lround(std::sin(aa)*rr); }
        // Dispersão determinística por carimbo evita custo/variação global de rand().
        if(brushScatterPercent>0.0f){ unsigned int seed=(unsigned int)(cx*73856093u)^(unsigned int)(cy*19349663u)^(unsigned int)(st*83492791u); seed=seed*1664525u+1013904223u; double jx=((seed&65535u)/32767.5-1.0)*(double)dw*brushScatterPercent/100.0; seed=seed*1664525u+1013904223u; double jy=((seed&65535u)/32767.5-1.0)*(double)dw*brushScatterPercent/100.0; cx+=(int)std::lround(jx);cy+=(int)std::lround(jy); }
        for(int oy=-halfY;oy<=halfY;++oy)for(int ox=-halfX;ox<=halfX;++ox){
            // Inverte a rotação e a compressão para consultar a máscara original.
            const double lx=(ox*cs+oy*sn)/(double)stampW+0.5;
            const double ly=(-(double)ox*sn+oy*cs)/(double)stampH+0.5;
            if(lx<0.0||ly<0.0||lx>=1.0||ly>=1.0)continue;
            const int mx=std::min(g_activeBrushWidth-1,(int)(lx*g_activeBrushWidth));
            const int my=std::min(g_activeBrushHeight-1,(int)(ly*g_activeBrushHeight));
            double coverage=(g_activeBrushMask[(size_t)my*g_activeBrushWidth+mx]/255.0);
            // O mesmo antialiasing global do lápis também suaviza máscaras de
            // brushes importados por amostragem bilinear, inclusive em rotação.
            if(lapisAntiAlias && g_activeBrushWidth>1 && g_activeBrushHeight>1) {
                double fx=lx*g_activeBrushWidth-0.5, fy=ly*g_activeBrushHeight-0.5;
                int x0=(int)std::floor(fx), y0=(int)std::floor(fy);
                double tx=fx-x0, ty=fy-y0;
                int xa=std::max(0,std::min(g_activeBrushWidth-1,x0));
                int xb=std::max(0,std::min(g_activeBrushWidth-1,x0+1));
                int ya=std::max(0,std::min(g_activeBrushHeight-1,y0));
                int yb=std::max(0,std::min(g_activeBrushHeight-1,y0+1));
                double a00=g_activeBrushMask[(size_t)ya*g_activeBrushWidth+xa];
                double a10=g_activeBrushMask[(size_t)ya*g_activeBrushWidth+xb];
                double a01=g_activeBrushMask[(size_t)yb*g_activeBrushWidth+xa];
                double a11=g_activeBrushMask[(size_t)yb*g_activeBrushWidth+xb];
                double top=a00+(a10-a00)*tx, bottom=a01+(a11-a01)*tx;
                coverage=std::max(0.0,std::min(1.0,(top+(bottom-top)*ty)/255.0));
            }
            if(blurAmount>0.001) {
                const int xm=std::max(0,mx-1), xp=std::min(g_activeBrushWidth-1,mx+1), ym=std::max(0,my-1), yp=std::min(g_activeBrushHeight-1,my+1);
                const double blur=(g_activeBrushMask[(size_t)ym*g_activeBrushWidth+xm]+2.0*g_activeBrushMask[(size_t)ym*g_activeBrushWidth+mx]+g_activeBrushMask[(size_t)ym*g_activeBrushWidth+xp]+2.0*g_activeBrushMask[(size_t)my*g_activeBrushWidth+xm]+4.0*g_activeBrushMask[(size_t)my*g_activeBrushWidth+mx]+2.0*g_activeBrushMask[(size_t)my*g_activeBrushWidth+xp]+g_activeBrushMask[(size_t)yp*g_activeBrushWidth+xm]+2.0*g_activeBrushMask[(size_t)yp*g_activeBrushWidth+mx]+g_activeBrushMask[(size_t)yp*g_activeBrushWidth+xp])/(16.0*255.0);
                coverage=coverage*(1.0-blurAmount)+blur*blurAmount;
            }
            double a=coverage*opacity;
            const int tx=cx+ox,ty=cy+oy;
            // Textura repetida em coordenadas do quadro; mantém-se fixa ao arrastar o pincel.
            if(!g_activeBrushTextureMask.empty() && g_activeBrushTextureWidth>0 && g_activeBrushTextureHeight>0) {
                int txu=tx%g_activeBrushTextureWidth, tyu=ty%g_activeBrushTextureHeight;
                if(txu<0)txu+=g_activeBrushTextureWidth; if(tyu<0)tyu+=g_activeBrushTextureHeight;
                a*=g_activeBrushTextureMask[(size_t)tyu*g_activeBrushTextureWidth+txu]/255.0;
            }
            int paintR=cr,paintG=cg,paintB=cb;
            if(!g_activeBrushTextureMask2.empty() && g_activeBrushTexture2Width>0 && g_activeBrushTexture2Height>0) { int ux=tx%g_activeBrushTexture2Width,uy=ty%g_activeBrushTexture2Height;if(ux<0)ux+=g_activeBrushTexture2Width;if(uy<0)uy+=g_activeBrushTexture2Height;double ta=(g_activeBrushTextureMask2[(size_t)uy*g_activeBrushTexture2Width+ux]/255.0)*(g_activeBrushTexture2Opacity/100.0);paintR=(int)(paintR*(1-ta)+GetRValue(g_activeBrushTexture2Tint)*ta);paintG=(int)(paintG*(1-ta)+GetGValue(g_activeBrushTexture2Tint)*ta);paintB=(int)(paintB*(1-ta)+GetBValue(g_activeBrushTexture2Tint)*ta); }
            if(!g_activeBrushTextureMask3.empty() && g_activeBrushTexture3Width>0 && g_activeBrushTexture3Height>0) { int ux=tx%g_activeBrushTexture3Width,uy=ty%g_activeBrushTexture3Height;if(ux<0)ux+=g_activeBrushTexture3Width;if(uy<0)uy+=g_activeBrushTexture3Height;double ta=(g_activeBrushTextureMask3[(size_t)uy*g_activeBrushTexture3Width+ux]/255.0)*(g_activeBrushTexture3Opacity/100.0);paintR=(int)(paintR*(1-ta)+GetRValue(g_activeBrushTexture3Tint)*ta);paintG=(int)(paintG*(1-ta)+GetGValue(g_activeBrushTexture3Tint)*ta);paintB=(int)(paintB*(1-ta)+GetBValue(g_activeBrushTexture3Tint)*ta); }
            if(g_brushesKodanima.size()>0 && g_brushSelecionado>=0 && g_brushSelecionado<(int)g_brushesKodanima.size()) { const BrushMetaKodanima& mm=g_brushesKodanima[g_brushSelecionado].meta; if(mm.startEndDynamics && st==0) a*=mm.startOpacityPercent/100.0; if(mm.startEndDynamics && st==steps) a*=mm.endOpacityPercent/100.0; }
            if(a<0.025)continue;
            // Anti-overflow: limita o acesso ao quadro/DC sem tentar escrever fora da região válida.
            if(antiOverflow && hasClip && (tx<clipRect.left||tx>=clipRect.right||ty<clipRect.top||ty>=clipRect.bottom)) continue;
            COLORREF oldc=GetPixel(dc,tx,ty); if(oldc==CLR_INVALID)continue;
            SetPixelV(dc,tx,ty,RGB((int)(GetRValue(oldc)*(1-a)+paintR*a),(int)(GetGValue(oldc)*(1-a)+paintG*a),(int)(GetBValue(oldc)*(1-a)+paintB*a)));
        }
        }
    }
}
static std::wstring MinusculaBrushText(std::wstring x);
static std::wstring ExtensaoBrush(const wchar_t* path);

static void RecarregarListaBrushes(HWND hwnd) {
    std::vector<BrushEntryKodanima> rebuilt;
    wchar_t folder[MAX_PATH]={0};
    if(!PastaGerenciadorBrushes(folder,MAX_PATH)) {
        if(hwnd && IsWindow(hwnd)) MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Biblioteca de pincéis",MB_OK|MB_ICONWARNING);
        return;
    }
    const std::wstring folderW=folder;
    std::map<std::wstring,BrushEntryKodanima> fromJson;
    std::wstring doc;
    if(LerDocumentoJsonBrush(doc)) {
        bool useCustom=true; if(JsonLerBoolBrush(doc,L"useImportedBrushes",useCustom)) g_useImportedBrushes=useCustom;
        size_t p=doc.find(L"\"brushes\"");
        if(p!=std::wstring::npos) {
            std::wstring obj; ++p;
            while(JsonExtrairObjetoBrush(doc,p,obj)) {
                std::wstring bmp,name,texture;
                if(JsonLerStringBrush(obj,L"bmp",bmp) && !bmp.empty()) {
                    // Suporta JSONs antigos que guardaram só o nome ou o caminho absoluto.
                    for(size_t k=0;k<bmp.size();++k) if(bmp[k]==L'/') bmp[k]=L'\\';
                    std::wstring base=bmp; size_t slash=base.find_last_of(L'\\'); if(slash!=std::wstring::npos) base=base.substr(slash+1);
                    std::wstring candidate=bmp;
                    if(!ArquivoExisteBrush(candidate)) { candidate=folderW+L"\\"+base; }
                    if(ArquivoExisteBrush(candidate)) {
                        JsonLerStringBrush(obj,L"name",name);
                        if(name.empty()) { name=base; size_t dot=name.find_last_of(L'.'); if(dot!=std::wstring::npos) name.resize(dot); }
                        BrushMetaKodanima meta=MetaPadraoImportacao(); LerMetaObjetoJsonBrush(obj,meta,texture);
                        BrushEntryKodanima e; e.name=name; e.bmpPath=candidate; e.meta=meta; e.texturePath=texture;
                        JsonLerStringBrush(obj,L"texture2",e.texture2Path); JsonLerStringBrush(obj,L"texture3",e.texture3Path);
                        double extra=0;
                        if(JsonLerNumeroBrush(obj,L"texture2Tint",extra)) e.texture2Tint=(COLORREF)(unsigned long)extra;
                        if(JsonLerNumeroBrush(obj,L"texture3Tint",extra)) e.texture3Tint=(COLORREF)(unsigned long)extra;
                        if(JsonLerNumeroBrush(obj,L"texture2Opacity",extra)) e.texture2Opacity=(float)std::max(0.0,std::min(100.0,extra));
                        if(JsonLerNumeroBrush(obj,L"texture3Opacity",extra)) e.texture3Opacity=(float)std::max(0.0,std::min(100.0,extra));
                        fromJson[MinusculaBrushText(base)]=e;
                    }
                }
            }
        }
    }
    // Primeiro inclui os registros válidos do JSON; a lista não depende mais de
    // FindFirstFile para descobrir os brushes que já estão cadastrados.
    for(std::map<std::wstring,BrushEntryKodanima>::const_iterator it=fromJson.begin();it!=fromJson.end();++it)
        rebuilt.push_back(it->second);
    // Em seguida descobre BMPs colocados manualmente na pasta e adiciona-os com defaults.
    std::wstring wildcard=folderW+L"\\*.*";
    WIN32_FIND_DATAW fd{}; HANDLE h=FindFirstFileW(wildcard.c_str(),&fd);
    if(h!=INVALID_HANDLE_VALUE) {
        do {
            if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring file=fd.cFileName;
            if(ExtensaoBrush(file.c_str())!=L".bmp") continue;
            bool already=false;
            for(size_t i=0;i<rebuilt.size();++i) if(_wcsicmp(rebuilt[i].bmpPath.c_str(),(folderW+L"\\"+file).c_str())==0) { already=true; break; }
            if(already) continue;
            BrushEntryKodanima e; e.name=file; size_t dot=e.name.find_last_of(L'.'); if(dot!=std::wstring::npos)e.name.resize(dot);
            e.bmpPath=folderW+L"\\"+file; e.meta=MetaPadraoImportacao(); rebuilt.push_back(e);
        } while(FindNextFileW(h,&fd));
        FindClose(h);
    }
    std::sort(rebuilt.begin(),rebuilt.end(),[](const BrushEntryKodanima& a,const BrushEntryKodanima& b){return _wcsicmp(a.name.c_str(),b.name.c_str())<0;});
    // Persist only after rebuilding from both sources; valid files stay visible even
    // when the JSON was missing or had paths written by an older version.
    if(!SalvarBibliotecaBrushesJson(rebuilt)) {
        // Keep the in-memory list usable. Report the failure only on operations that save.
    }
    g_brushesKodanima.swap(rebuilt);
    HWND list=hwnd?GetDlgItem(hwnd,ID_BRUSH_LIST):NULL;
    if(list) {
        SendMessageW(list,LB_RESETCONTENT,0,0);
        for(size_t i=0;i<g_brushesKodanima.size();++i) {
            LRESULT added=SendMessageW(list,LB_ADDSTRING,0,(LPARAM)g_brushesKodanima[i].name.c_str());
            if(added==LB_ERR || added==LB_ERRSPACE) {
                // A listbox that failed to allocate its item storage should not silently
                // produce an empty-looking panel.
                SetWindowTextW(hwnd,L"Pincéis do Kodanima — erro ao montar lista");
                break;
            }
        }
        if(!g_brushesKodanima.empty()) {
            int sel=std::max(0,std::min((int)g_brushesKodanima.size()-1,g_brushSelecionado));
            SendMessageW(list,LB_SETCURSEL,sel,0); AtivarBrushSelecionado(sel);
            HWND nameEdit=GetDlgItem(hwnd,ID_BRUSH_NAME_EDIT); if(nameEdit) SetWindowTextW(nameEdit,g_brushesKodanima[sel].name.c_str());
        } else {
            g_brushSelecionado=-1;
            if(g_activeBrushBitmap){DeleteObject(g_activeBrushBitmap);g_activeBrushBitmap=NULL;}
            g_activeBrushMask.clear();g_activeBrushWidth=g_activeBrushHeight=0;g_activeBrushPath.clear();
            g_activeBrushTextureMask.clear();g_activeBrushTextureWidth=g_activeBrushTextureHeight=0;g_activeBrushTexturePath.clear();
        }
    }
    if(hwnd) InvalidateRect(hwnd,NULL,TRUE);
}

static bool GravarMetaBrush(const BrushEntryKodanima& e,const BrushMetaKodanima* preset=NULL) {
    BrushMetaKodanima m{};
    if(preset) m=*preset;
    else {
        m=MetaPadraoImportacao();
        m.size=tamanhoLapis; m.minSize=tamanhoMinimoPressao; m.maxSize=tamanhoMaximoPressao;
        m.hardness=durezaPincel; m.antialias=lapisAntiAlias?1:0;
        m.spacingPercent=brushSpacingPercent; m.angleDegrees=brushAngleDegrees; m.roundnessPercent=brushRoundnessPercent; m.scatterPercent=brushScatterPercent;
        m.opacityPercent=brushOpacityPercent; m.flowPercent=brushFlowPercent;
        m.rotateWithStroke=brushRotateWithStroke?1:0; m.sizeByPressure=brushSizeByPressure?1:0; m.opacityByPressure=brushOpacityByPressure?1:0;
    }
    if(!AtualizarRegistroBrushJson(e,&m,NULL)) {
        if(g_brushStorageError.empty()) DefinirErroBrush(L"Não foi possível gravar os parâmetros do pincel no JSON mestre",e.bmpPath.c_str(),ERROR_WRITE_FAULT);
        return false;
    }
    return true;
}
// Leitura de arquivos de configuração de recursos. SAI 1 usa linhas
// "tipo,caminho_relativo" em brushform.conf e brushtex.conf. MediBang guarda
// bitmapFile/name/width/opacity/dinâmicas em Brush2.ini, com imagens em brush_bitmap.
static std::wstring TrimBrushText(std::wstring x) {
    while(!x.empty() && (x.front()==L' '||x.front()==L'\t'||x.front()==L'\r'||x.front()==L'\n')) x.erase(x.begin());
    while(!x.empty() && (x.back()==L' '||x.back()==L'\t'||x.back()==L'\r'||x.back()==L'\n')) x.pop_back();
    if(x.size()>=2 && ((x.front()==L'"'&&x.back()==L'"')||(x.front()==L'\''&&x.back()==L'\''))) { x=x.substr(1,x.size()-2); }
    return x;
}
static std::wstring MinusculaBrushText(std::wstring x) { for(size_t i=0;i<x.size();++i) x[i]=(wchar_t)towlower(x[i]); return x; }
static std::wstring LerArquivoTextoBrush(const wchar_t* path) {
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) return std::wstring();
    LARGE_INTEGER size{}; if(!GetFileSizeEx(f,&size)||size.QuadPart<=0||size.QuadPart>8*1024*1024) { CloseHandle(f); return std::wstring(); }
    std::vector<char> bytes((size_t)size.QuadPart); DWORD got=0; BOOL ok=ReadFile(f,bytes.data(),(DWORD)bytes.size(),&got,NULL); CloseHandle(f);
    if(!ok||got!=bytes.size()) return std::wstring();
    if(bytes.size()>=2 && (unsigned char)bytes[0]==0xFF && (unsigned char)bytes[1]==0xFE) {
        size_t chars=(bytes.size()-2)/sizeof(wchar_t); std::wstring out(chars,L'\0');
        if(chars) memcpy(&out[0],bytes.data()+2,chars*sizeof(wchar_t));
        return out;
    }
    size_t off=bytes.size()>=3 && (unsigned char)bytes[0]==0xEF && (unsigned char)bytes[1]==0xBB && (unsigned char)bytes[2]==0xBF ? 3 : 0;
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data()+off,(int)(bytes.size()-off),NULL,0);
    UINT cp=CP_UTF8; DWORD flags=MB_ERR_INVALID_CHARS;
    if(n<=0) { cp=CP_ACP; flags=0; n=MultiByteToWideChar(cp,flags,bytes.data()+off,(int)(bytes.size()-off),NULL,0); }
    if(n<=0) return std::wstring();
    std::wstring out((size_t)n,L'\0'); MultiByteToWideChar(cp,flags,bytes.data()+off,(int)(bytes.size()-off),&out[0],n); return out;
}
static std::wstring DiretorioPaiBrush(const wchar_t* path) { std::wstring p=path?path:L""; size_t q=p.find_last_of(L"\\/"); if(q==std::wstring::npos)return L"."; if(q==0)return p.substr(0,1); return p.substr(0,q); }
static bool ArquivoExisteBrush(const std::wstring& path);
static std::wstring JuntarCaminhoBrush(const std::wstring& base,const std::wstring& child) {
    if(child.size()>=2 && child[1]==L':' ) return child;
    if(child.size()>=2 && child[0]==L'\\'&&child[1]==L'\\') return child;
    std::wstring c=child; for(size_t i=0;i<c.size();++i)if(c[i]==L'/')c[i]=L'\\';
    std::wstring b=base; while(!b.empty()&&(b.back()==L'\\'||b.back()==L'/'))b.pop_back();
    if(!c.empty()&&c[0]==L'\\')c.erase(c.begin());
    return b+L"\\"+c;
}
static std::wstring ResolverRecursoRelativoBrush(const std::wstring& base,const std::wstring& relative) {
    if((relative.size()>1&&relative[1]==L':') || (relative.size()>1&&relative[0]==L'\\'&&relative[1]==L'\\')) return relative;
    std::wstring dir=base;
    // Alguns pacotes guardam o .conf/.ini dentro de uma subpasta, mas os recursos
    // são relativos à raiz de instalação ou à pasta-irmã. Procura até quatro níveis.
    for(int level=0;level<4&&!dir.empty();++level) {
        std::wstring cand=JuntarCaminhoBrush(dir,relative);
        if(ArquivoExisteBrush(cand)) return cand;
        if(level==0) {
            std::wstring slash=relative; for(size_t i=0;i<slash.size();++i)if(slash[i]==L'/')slash[i]=L'\\';
            if(slash.find(L"brush_bitmap\\")!=0) {
                cand=JuntarCaminhoBrush(JuntarCaminhoBrush(dir,L"brush_bitmap"),slash);
                if(ArquivoExisteBrush(cand)) return cand;
            }
        }
        std::wstring parent=DiretorioPaiBrush(dir.c_str());
        if(parent==dir || parent.empty()) break;
        dir=parent;
    }
    return JuntarCaminhoBrush(base,relative);
}
static std::wstring ExtensaoBrush(const wchar_t* path) { const wchar_t* p=path?wcsrchr(path,L'.'):NULL; return p?MinusculaBrushText(p):L""; }
static bool ArquivoExisteBrush(const std::wstring& path) { DWORD a=GetFileAttributesW(path.c_str()); return a!=INVALID_FILE_ATTRIBUTES && !(a&FILE_ATTRIBUTE_DIRECTORY); }
static bool CriarNomeImagemBrush(const std::wstring& folder,const std::wstring& base,std::wstring& dest) {
    std::wstring stem=NomeArquivoSeguroBrush(base); dest=folder+L"\\"+stem+L".bmp"; unsigned int serial=2;
    while(GetFileAttributesW(dest.c_str())!=INVALID_FILE_ATTRIBUTES && serial<100000) { wchar_t suffix[32]; swprintf(suffix,32,L"_%u",serial++); dest=folder+L"\\"+stem+suffix+L".bmp"; }
    if(GetFileAttributesW(dest.c_str())!=INVALID_FILE_ATTRIBUTES) { DefinirErroBrush(L"Não foi possível gerar um nome exclusivo para o pincel",dest.c_str(),ERROR_FILE_EXISTS); return false; }
    return true;
}
static bool ImportarImagemRasterizada(const wchar_t* source,const std::wstring& folder,const std::wstring& destPrefix,bool asBrush,BrushMetaKodanima* preset,std::wstring& writtenPath) {
    g_brushStorageError.clear(); g_brushStorageWin32Error=ERROR_SUCCESS;
    int w=0,h=0; std::vector<unsigned char> mask; std::wstring detail;
    if(!CarregarImagemComoMascara(source,w,h,mask,&detail)) { if(detail.empty()) detail=L"Formato de imagem inválido."; g_brushStorageError=detail+L"\nArquivo: "+source; return false; }
    const wchar_t* name=wcsrchr(source,L'\\'); name=name?name+1:source; std::wstring sourceStem=name; size_t dot=sourceStem.find_last_of(L'.'); if(dot!=std::wstring::npos)sourceStem.resize(dot);
    std::wstring displayName=NomeArquivoSeguroBrush(destPrefix+sourceStem);
    std::wstring stem=displayName;
    if(!CriarNomeImagemBrush(folder,stem,writtenPath)) return false;
    if(!EscreverMascaraBMP(writtenPath.c_str(),w,h,mask)) { DWORD e=GetLastError(); DefinirErroBrush(L"Não foi possível gravar a imagem normalizada do pincel/textura",writtenPath.c_str(),e); return false; }
    if(asBrush) {
        BrushEntryKodanima e; e.bmpPath=writtenPath; e.name=displayName; e.meta=preset?*preset:MetaPadraoImportacao();
        if(!GravarMetaBrush(e,preset)) { DeleteFileW(writtenPath.c_str()); return false; }
    }
    return true;
}
static BrushMetaKodanima MetaPadraoImportacao() {
    BrushMetaKodanima m{};
    m.size=tamanhoLapis; m.minSize=tamanhoMinimoPressao; m.maxSize=tamanhoMaximoPressao; m.hardness=durezaPincel; m.antialias=lapisAntiAlias?1:0;
    m.spacingPercent=brushSpacingPercent; m.angleDegrees=brushAngleDegrees; m.roundnessPercent=brushRoundnessPercent; m.scatterPercent=brushScatterPercent;
    m.opacityPercent=brushOpacityPercent; m.flowPercent=brushFlowPercent; m.rotateWithStroke=brushRotateWithStroke?1:0;
    m.sizeByPressure=brushSizeByPressure?1:0; m.opacityByPressure=brushOpacityByPressure?1:0;
    m.startSizePercent=100.0f; m.endSizePercent=100.0f; m.startOpacityPercent=100.0f; m.endOpacityPercent=100.0f;
    m.sprayDensity=10.0f; m.sprayRadiusPercent=80.0f; m.particleCount=4; m.particleSpreadPercent=60.0f;
    m.scaleRandomPercent=0.0f; m.blurPercent=0.0f;
    m.sprayingEffect=0; m.randomRotation=0; m.followPenDirection=1; m.antiOverflow=1; m.optimizeStroke=0; m.startEndDynamics=0; m.particlesEnabled=0; return m;
}
static int ImportarConfigSAI(HWND hwnd,const wchar_t* confPath,bool asTexture) {
    std::wstring text=LerArquivoTextoBrush(confPath);
    if(text.empty()) { MessageBoxW(hwnd,L"Não consegui ler o arquivo de configuração SAI. Verifique se é um brushform.conf ou brushtex.conf válido.",L"Importar recursos do SAI",MB_OK|MB_ICONWARNING); return 0; }
    std::wstring root=DiretorioPaiBrush(confPath), textureFolder, brushFolder; wchar_t tmp[MAX_PATH]={0};
    if(!PastaGerenciadorBrushes(tmp,MAX_PATH)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Biblioteca de pincéis",MB_OK|MB_ICONERROR); return 0; }
    brushFolder=tmp; if(asTexture && !PastaTexturasBrushes(textureFolder)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Biblioteca de texturas",MB_OK|MB_ICONERROR); return 0; }
    std::wistringstream lines(text); std::wstring line; int imported=0,missing=0,seen=0; std::wstring lastFailure;
    while(std::getline(lines,line) && seen<2000) {
        line=TrimBrushText(line); if(line.empty()||line[0]==L'#'||line[0]==L';')continue;
        size_t comma=line.find(L','); if(comma==std::wstring::npos)continue;
        int type=_wtoi(TrimBrushText(line.substr(0,comma)).c_str()); std::wstring rel=TrimBrushText(line.substr(comma+1));
        if(rel.empty()||rel.find(L':')!=std::wstring::npos)continue;
        // SAI: 1 = forma de borda/textura cinza; 2 = mapa de cerdas 63x63.
        if(asTexture && type!=1) continue; if(!asTexture && type!=1 && type!=2)continue;
        std::wstring source=ResolverRecursoRelativoBrush(root,rel); ++seen;
        if(!ArquivoExisteBrush(source)) { ++missing; continue; }
        std::wstring destination;
        const std::wstring prefix=asTexture?L"SAI_Textura_":(type==2?L"SAI_Cerdas_":L"SAI_Forma_");
        BrushMetaKodanima meta=MetaPadraoImportacao();
        bool ok=ImportarImagemRasterizada(source.c_str(),asTexture?textureFolder:brushFolder,prefix,!asTexture,&meta,destination);
        if(ok)++imported; else { ++missing; if(!g_brushStorageError.empty()) lastFailure=g_brushStorageError; }
    }
    if(!asTexture) RecarregarListaBrushes(hwnd);
    wchar_t msg[1024]; swprintf(msg,1024,L"Importação SAI concluída.\nRecursos importados: %d\nLinhas com arquivo ausente/erro: %d\n\n%s",imported,missing,asTexture?L"As texturas estão na subpasta texturas da biblioteca. Use Textura... na janela de pincéis para associá-las a um pincel.":L"Formas tipo 1 e mapas de cerdas tipo 2 foram convertidos para pontas BMP internas do Kodanima.");
    std::wstring fullMessage=msg; if(!lastFailure.empty()) { fullMessage+=L"\n\nÚltimo erro de importação/gravação:\n"; fullMessage+=lastFailure; }
    MessageBoxW(hwnd,fullMessage.c_str(),L"Recursos PaintTool SAI",imported?MB_OK|MB_ICONINFORMATION:MB_OK|MB_ICONWARNING); return imported;
}
struct MediBangBrushInfo {
    std::wstring name,bitmapFile,type;
    double width=32.0,minimum=0.0,opacity=1.0;
    bool pressureSize=true,pressureAlpha=false;
};
static bool TextoTrueBrush(const std::wstring& v) { std::wstring x=MinusculaBrushText(TrimBrushText(v)); return x==L"true"||x==L"1"||x==L"yes"||x==L"on"; }
static int ImportarConfigMediBang(HWND hwnd,const wchar_t* iniPath) {
    std::wstring text=LerArquivoTextoBrush(iniPath);
    if(text.empty()) { MessageBoxW(hwnd,L"Não consegui ler Brush2.ini. No MediBang, abra Ajuda > Abrir pasta de configuração e selecione o Brush2.ini dessa pasta.",L"Importar MediBang",MB_OK|MB_ICONWARNING); return 0; }
    const std::wstring root=DiretorioPaiBrush(iniPath); wchar_t out[MAX_PATH]={0};
    if(!PastaGerenciadorBrushes(out,MAX_PATH)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Biblioteca de pincéis",MB_OK|MB_ICONERROR); return 0; }
    const std::wstring destFolder=out; std::wistringstream lines(text); std::wstring line; MediBangBrushInfo item; bool inSection=false; int imported=0,missing=0,sections=0; std::wstring lastFailure;
    auto process=[&](const MediBangBrushInfo& b) {
        if(b.bitmapFile.empty() || (!b.type.empty() && MinusculaBrushText(b.type)!=L"bitmap")) return;
        if(++sections>3000) return;
        std::wstring rel=b.bitmapFile; for(size_t i=0;i<rel.size();++i)if(rel[i]==L'/')rel[i]=L'\\';
        std::wstring source;
        if(rel.size()>1 && rel[1]==L':') source=rel;
        else if(rel.size()>1 && rel[0]==L'\\'&&rel[1]==L'\\') source=rel;
        else {
            std::wstring dir=root;
            for(int level=0;level<4&&!ArquivoExisteBrush(source);++level) {
                std::wstring lowered=MinusculaBrushText(rel);
                if(lowered.find(L"brush_bitmap\\")==0) source=JuntarCaminhoBrush(dir,rel);
                else source=JuntarCaminhoBrush(JuntarCaminhoBrush(dir,L"brush_bitmap"),rel);
                if(!ArquivoExisteBrush(source)) source=JuntarCaminhoBrush(dir,rel);
                if(ArquivoExisteBrush(source)) break;
                std::wstring parent=DiretorioPaiBrush(dir.c_str()); if(parent==dir||parent.empty()) break; dir=parent;
            }
        }
        if(!ArquivoExisteBrush(source)) { ++missing; return; }
        BrushMetaKodanima m=MetaPadraoImportacao();
        double width=std::max(1.0,std::min(5000.0,b.width));
        m.size=(float)width; m.minSize=(float)std::max(1.0,std::min(width,b.minimum>1.0?width*b.minimum/100.0:width*b.minimum));
        m.maxSize=(float)std::max((double)m.minSize,width); m.opacityPercent=(float)std::max(0.0,std::min(100.0,b.opacity<=1.0?b.opacity*100.0:b.opacity));
        m.sizeByPressure=b.pressureSize?1:0; m.opacityByPressure=b.pressureAlpha?1:0;
        std::wstring brushName=b.name.empty()?L"MediBang":b.name;
        std::wstring dest;
        if(ImportarImagemRasterizada(source.c_str(),destFolder,L"MediBang_"+brushName,true,&m,dest)) ++imported; else { ++missing; if(!g_brushStorageError.empty()) lastFailure=g_brushStorageError; }
    };
    while(std::getline(lines,line)) {
        line=TrimBrushText(line); if(line.empty()||line[0]==L';'||line[0]==L'#')continue;
        if(line.front()==L'['&&line.back()==L']') { if(inSection)process(item); item=MediBangBrushInfo(); inSection=true; continue; }
        size_t eq=line.find(L'='); if(eq==std::wstring::npos)continue;
        std::wstring key=MinusculaBrushText(TrimBrushText(line.substr(0,eq))), val=TrimBrushText(line.substr(eq+1));
        if(key==L"name")item.name=val; else if(key==L"type")item.type=val; else if(key==L"bitmapfile")item.bitmapFile=val;
        else if(key==L"width")item.width=wcstod(val.c_str(),NULL); else if(key==L"min")item.minimum=wcstod(val.c_str(),NULL);
        else if(key==L"opacity")item.opacity=wcstod(val.c_str(),NULL); else if(key==L"psize")item.pressureSize=TextoTrueBrush(val); else if(key==L"palpha")item.pressureAlpha=TextoTrueBrush(val);
    }
    if(inSection)process(item);
    RecarregarListaBrushes(hwnd);
    wchar_t msg[700]; swprintf(msg,700,L"Importação MediBang concluída.\nPontas bitmap importadas: %d\nReferências de imagem ausentes/erros: %d\n\nAs opções width, min, opacity, pSize e pAlpha de Brush2.ini foram convertidas para parâmetros equivalentes do Kodanima. Scripts .bs e efeitos específicos do MediBang não são executados pelo Kodanima.",imported,missing);
    std::wstring fullMessage=msg; if(!lastFailure.empty()) { fullMessage+=L"\n\nÚltimo erro de importação/gravação:\n"; fullMessage+=lastFailure; }
    MessageBoxW(hwnd,fullMessage.c_str(),L"Recursos MediBang Paint",imported?MB_OK|MB_ICONINFORMATION:MB_OK|MB_ICONWARNING); return imported;
}
static bool AssociarTexturaAoPincel(HWND hwnd) {
    int ix=(int)SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_GETCURSEL,0,0);
    if(ix<0||ix>=(int)g_brushesKodanima.size()) { MessageBoxW(hwnd,L"Selecione primeiro um pincel na lista.",L"Textura",MB_OK|MB_ICONINFORMATION); return false; }
    const BrushEntryKodanima e=g_brushesKodanima[ix];
    if(!g_activeBrushTexturePath.empty()) {
        int answer=MessageBoxW(hwnd,L"Este pincel já tem uma textura.\nSim: remover a textura.\nNão: escolher/substituir a textura.",L"Textura do pincel",MB_YESNOCANCEL|MB_ICONQUESTION);
        if(answer==IDCANCEL) return false;
        if(answer==IDYES) {
            if(!SalvarAssociacaoTextura(e,L"")) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Não foi possível remover a textura",MB_OK|MB_ICONERROR); return false; }
            g_activeBrushTextureMask.clear(); g_activeBrushTexturePath.clear(); g_activeBrushTextureWidth=g_activeBrushTextureHeight=0; InvalidateRect(hwnd,NULL,TRUE); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE); return true;
        }
    }
    std::wstring textureFolder; if(!PastaTexturasBrushes(textureFolder)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Biblioteca de texturas",MB_OK|MB_ICONERROR); return false; }
    wchar_t selected[MAX_PATH]={0}; OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hwnd; ofn.lpstrFile=selected; ofn.nMaxFile=MAX_PATH;
    ofn.lpstrFilter=L"Imagens de textura (*.bmp;*.png)\0*.bmp;*.png\0BMP (*.bmp)\0*.bmp\0PNG (*.png)\0*.png\0Todos os arquivos (*.*)\0*.*\0\0";
    ofn.lpstrInitialDir=textureFolder.c_str(); ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY; const std::wstring title=KodaTraduzirTexto(L"Escolher textura para este pincel"); ofn.lpstrTitle=title.c_str();
    if(!GetOpenFileNameW(&ofn)) return false;
    const std::wstring ext=ExtensaoBrush(selected); if(ext!=L".bmp"&&ext!=L".png") { MessageBoxW(hwnd,L"Selecione uma imagem BMP ou PNG.",L"Textura",MB_OK|MB_ICONWARNING); return false; }
    std::wstring dest; if(!ImportarImagemRasterizada(selected,textureFolder,L"Textura_",false,NULL,dest)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Não foi possível importar a textura",MB_OK|MB_ICONERROR); return false; }
    if(!SalvarAssociacaoTextura(e,dest)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Não foi possível associar a textura",MB_OK|MB_ICONERROR); DeleteFileW(dest.c_str()); return false; }
    int tw=0,th=0; std::vector<unsigned char> tm; std::wstring why;
    if(CarregarImagemComoMascara(dest.c_str(),tw,th,tm,&why)) { g_activeBrushTexturePath=dest; g_activeBrushTextureWidth=tw; g_activeBrushTextureHeight=th; g_activeBrushTextureMask.swap(tm); }
    InvalidateRect(hwnd,NULL,TRUE); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE);
    MessageBoxW(hwnd,L"Textura importada e vinculada ao pincel. O Kodanima aplica a máscara de textura repetida durante o traço.",L"Textura vinculada",MB_OK|MB_ICONINFORMATION); return true;
}
static void ImportarBrush(HWND hwnd) {
    wchar_t selected[MAX_PATH]={0}; OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hwnd; ofn.lpstrFile=selected; ofn.nMaxFile=MAX_PATH;
    ofn.lpstrFilter=L"Imagem de pincel BMP/PNG (*.bmp;*.png)\0*.bmp;*.png\0Configuração de forma SAI (brushform.conf)\0brushform.conf\0Configuração de textura SAI (brushtex.conf)\0brushtex.conf\0Configuração de textura de papel SAI (papertex.conf)\0papertex.conf\0Pincéis MediBang (Brush2.ini)\0Brush2.ini\0Arquivo MDP do MediBang (não decodificado diretamente)\0*.mdp\0Todos os arquivos (*.*)\0*.*\0\0";
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY; const std::wstring title=KodaTraduzirTexto(L"Importar pincel ou recursos de outro programa"); ofn.lpstrTitle=title.c_str();
    if(!GetOpenFileNameW(&ofn)) return;
    std::wstring ext=ExtensaoBrush(selected), basename=MinusculaBrushText(wcsrchr(selected,L'\\')?wcsrchr(selected,L'\\')+1:selected);
    if(ext==L".conf") {
        bool texture=(basename==L"brushtex.conf"||basename==L"papertex.conf");
        if(basename!=L"brushform.conf"&&!texture) { MessageBoxW(hwnd,L"Selecione brushform.conf para importar formas ou brushtex.conf/papertex.conf para importar texturas SAI.",L"Configuração SAI",MB_OK|MB_ICONWARNING); return; }
        ImportarConfigSAI(hwnd,selected,texture); return;
    }
    if(ext==L".ini") {
        if(basename!=L"brush2.ini") { MessageBoxW(hwnd,L"O importador do MediBang espera o arquivo Brush2.ini da pasta de configuração. No MediBang, use Ajuda > Abrir pasta de configuração.",L"Configuração MediBang",MB_OK|MB_ICONWARNING); return; }
        ImportarConfigMediBang(hwnd,selected); return;
    }
    if(ext==L".mdp") {
        MessageBoxW(hwnd,L"O arquivo .mdp é o contêiner nativo do MediBang e também pode conter várias camadas de uma ponta bitmap. A documentação pública consultada descreve o fluxo dentro do MediBang, mas não especifica o formato binário para um leitor independente confiável.\n\nPara importar as pontas usando este código: adicione o MDP no MediBang (Adicionar pincel bitmap > De arquivo) e depois importe o Brush2.ini em Ajuda > Abrir pasta de configuração; ou exporte a ponta como PNG. O Kodanima lê os bitmapFile referenciados no Brush2.ini e converte cada ponta para sua biblioteca própria.",L"Importação MDP",MB_OK|MB_ICONINFORMATION); return;
    }
    if(ext!=L".bmp"&&ext!=L".png") { MessageBoxW(hwnd,L"Formato não suportado. Use BMP/PNG, brushform.conf, brushtex.conf ou Brush2.ini.",L"Importar pincel",MB_OK|MB_ICONWARNING); return; }
    wchar_t folder[MAX_PATH]={0}; if(!PastaGerenciadorBrushes(folder,MAX_PATH)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Falha ao salvar pincel",MB_OK|MB_ICONERROR); return; }
    std::wstring dest; BrushMetaKodanima meta=MetaPadraoImportacao();
    if(!ImportarImagemRasterizada(selected,folder,L"",true,&meta,dest)) { MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Falha ao importar pincel",MB_OK|MB_ICONERROR); return; }
    RecarregarListaBrushes(hwnd);
    if(!g_brushesKodanima.empty()) {
        int chosen=-1; for(size_t i=0;i<g_brushesKodanima.size();++i) if(_wcsicmp(g_brushesKodanima[i].bmpPath.c_str(),dest.c_str())==0) { chosen=(int)i; break; }
        if(chosen>=0) { g_brushSelecionado=chosen; SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_SETCURSEL,chosen,0); AtivarBrushSelecionado(chosen); SetWindowTextW(GetDlgItem(hwnd,ID_BRUSH_NAME_EDIT),g_brushesKodanima[chosen].name.c_str()); }
    }
    InvalidateRect(hwnd,NULL,TRUE); MessageBoxW(hwnd,L"Ponta BMP/PNG normalizada e salva na biblioteca do Kodanima. Os parâmetros e o vínculo da textura ficam no JSON mestre brushes.json; a imagem foi convertida para máscara de cobertura.",L"Pincel importado",MB_OK|MB_ICONINFORMATION);
}

static void AbrirPropriedadesBrush(HWND owner,bool mechanics);
static void AbrirGerenciadorPincel(HWND owner) {
    if(hwndBrushManager && IsWindow(hwndBrushManager)){ ShowWindow(hwndBrushManager,SW_SHOWNORMAL); SetForegroundWindow(hwndBrushManager); return; }
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=WndProcBrushManager; wc.hInstance=hInstGlobal; wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512)); wc.hIcon=hIconeApp; wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1); wc.lpszClassName=L"KodanimaBrushManagerClass"; RegisterClassExW(&wc);
    RECT wr{},wa{}; GetWindowRect(owner?owner:hwndCanvas,&wr); SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0); int x=wr.right+8,y=wr.top; if(x+540>wa.right) x=std::max((int)wa.left,(int)wr.left-548); if(y+480>wa.bottom) y=std::max((int)wa.top,(int)wa.bottom-488); if(y<wa.top)y=wa.top;
    hwndBrushManager=CreateWindowExW(WS_EX_TOOLWINDOW,L"KodanimaBrushManagerClass",L"Pincéis do Kodanima",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME,x,y,540,480,owner,NULL,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"+",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,12,8,34,27,hwndBrushManager,(HMENU)ID_BRUSH_IMPORT,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Pincéis importados",WS_VISIBLE|WS_CHILD,54,11,190,22,hwndBrushManager,NULL,hInstGlobal,NULL);
    CreateWindowW(L"LISTBOX",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|LBS_NOTIFY|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED|WS_VSCROLL,12,40,235,260,hwndBrushManager,(HMENU)ID_BRUSH_LIST,hInstGlobal,NULL);
    CreateWindowW(L"STATIC",L"Prévia do pincel",WS_VISIBLE|WS_CHILD,270,12,180,22,hwndBrushManager,NULL,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"X Excluir",WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,12,350,95,28,hwndBrushManager,(HMENU)ID_BRUSH_DELETE,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"⚙ Propriedades",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,115,350,135,28,hwndBrushManager,(HMENU)ID_BRUSH_SETTINGS,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Textura...",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,260,350,105,28,hwndBrushManager,(HMENU)ID_BRUSH_TEXTURE,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Fechar",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,425,350,85,28,hwndBrushManager,(HMENU)ID_BRUSH_CLOSE,hInstGlobal,NULL);
    CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,12,310,235,25,hwndBrushManager,(HMENU)ID_BRUSH_NAME_EDIT,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Renomear",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,260,310,105,25,hwndBrushManager,(HMENU)ID_BRUSH_RENAME,hInstGlobal,NULL);
    CreateWindowW(L"BUTTON",L"Mecânicas (MKV)",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,370,310,140,25,hwndBrushManager,(HMENU)ID_BRUSH_MKV,hInstGlobal,NULL);
    HWND useImported=CreateWindowW(L"BUTTON",L"Pincéis importados: ATIVADOS (desmarque para usar o pincel padrão da engine)",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,12,390,490,24,hwndBrushManager,(HMENU)ID_BRUSH_USE_IMPORTED,hInstGlobal,NULL);
    RecarregarListaBrushes(hwndBrushManager);
    SendMessageW(useImported,BM_SETCHECK,g_useImportedBrushes?BST_CHECKED:BST_UNCHECKED,0);
    ShowWindow(hwndBrushManager,SW_SHOWNORMAL); UpdateWindow(hwndBrushManager);
}
LRESULT CALLBACK WndProcBrushManager(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(msg==WM_COMMAND){ int id=LOWORD(wParam);
        if(id==ID_BRUSH_USE_IMPORTED){
            g_useImportedBrushes=SendMessageW(GetDlgItem(hwnd,ID_BRUSH_USE_IMPORTED),BM_GETCHECK,0,0)==BST_CHECKED;
            if(!g_useImportedBrushes) g_brushPendingSegments.clear();
            if(!SalvarBibliotecaBrushesJson(g_brushesKodanima)) MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Salvar preferência de pincéis",MB_OK|MB_ICONWARNING);
            if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE); InvalidateRect(hwnd,NULL,FALSE); return 0;
        }
        if(id==ID_BRUSH_IMPORT){ImportarBrush(hwnd);return 0;} if(id==ID_BRUSH_TEXTURE){AssociarTexturaAoPincel(hwnd);return 0;} if(id==ID_BRUSH_CLOSE){DestroyWindow(hwnd);return 0;}
        if(id==ID_BRUSH_LIST && HIWORD(wParam)==LBN_SELCHANGE){int ix=(int)SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_GETCURSEL,0,0);AtivarBrushSelecionado(ix); if(ix>=0&&ix<(int)g_brushesKodanima.size()) SetWindowTextW(GetDlgItem(hwnd,ID_BRUSH_NAME_EDIT),g_brushesKodanima[ix].name.c_str()); InvalidateRect(hwnd,NULL,TRUE);return 0;}
        if(id==ID_BRUSH_DELETE){ int ix=(int)SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_GETCURSEL,0,0); if(ix>=0&&ix<(int)g_brushesKodanima.size()){ if(MessageBoxW(hwnd,L"Excluir este pincel importado?",L"Kodanima",MB_YESNO|MB_ICONQUESTION)==IDYES){DeleteFileW(g_brushesKodanima[ix].bmpPath.c_str());g_brushSelecionado=std::max(0,ix-1);RecarregarListaBrushes(hwnd);} } return 0; }
        if(id==ID_BRUSH_SETTINGS){ AbrirPropriedadesBrush(hwnd,false); return 0; } if(id==ID_BRUSH_MKV){ AbrirPropriedadesBrush(hwnd,true); return 0; } if(id==ID_BRUSH_RENAME){ int ix=(int)SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_GETCURSEL,0,0); if(ix>=0&&ix<(int)g_brushesKodanima.size()){ wchar_t nm[256]={0}; GetWindowTextW(GetDlgItem(hwnd,ID_BRUSH_NAME_EDIT),nm,256); std::wstring safe=NomeArquivoSeguroBrush(nm); g_brushesKodanima[ix].name=safe; if(AtualizarRegistroBrushJson(g_brushesKodanima[ix],NULL,NULL)){ RecarregarListaBrushes(hwnd); SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_SETCURSEL,ix,0); SetWindowTextW(GetDlgItem(hwnd,ID_BRUSH_NAME_EDIT),safe.c_str()); } else MessageBoxW(hwnd,g_brushStorageError.c_str(),L"Renomear pincel",MB_OK|MB_ICONERROR); } return 0; }
    }
    if(msg==WM_MEASUREITEM) {
        MEASUREITEMSTRUCT* mi=(MEASUREITEMSTRUCT*)lParam;
        if(mi && mi->CtlID==ID_BRUSH_LIST) { mi->itemHeight=44; return TRUE; }
    }
    if(msg==WM_DRAWITEM) {
        DRAWITEMSTRUCT* d=(DRAWITEMSTRUCT*)lParam;
        if(d && d->CtlID==ID_BRUSH_LIST) {
            if(d->itemID==(UINT)-1 || d->itemID>=g_brushesKodanima.size()) return TRUE;
            const bool selected=(d->itemState&ODS_SELECTED)!=0;
            COLORREF bg=selected?RGB(48,75,120):(temaAtual==1?RGB(250,250,252):RGB(36,38,44));
            COLORREF fg=(temaAtual==1&&!selected)?RGB(25,27,32):RGB(235,237,242);
            HBRUSH br=CreateSolidBrush(bg); FillRect(d->hDC,&d->rcItem,br); DeleteObject(br);
            RECT thumb={d->rcItem.left+4,d->rcItem.top+3,d->rcItem.left+42,d->rcItem.bottom-3};
            HBRUSH thumbBg=CreateSolidBrush(RGB(18,19,23)); FillRect(d->hDC,&thumb,thumbBg); DeleteObject(thumbBg);
            HBITMAP hb=(HBITMAP)LoadImageW(NULL,g_brushesKodanima[d->itemID].bmpPath.c_str(),IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
            if(hb) { BITMAP bm{}; if(GetObjectW(hb,sizeof(bm),&bm)) { HDC mem=CreateCompatibleDC(d->hDC); if(mem) { HGDIOBJ old=SelectObject(mem,hb); double scale=std::min((double)(thumb.right-thumb.left-2)/(double)std::max(1,(int)bm.bmWidth),(double)(thumb.bottom-thumb.top-2)/(double)std::max(1,(int)bm.bmHeight)); int dw=std::max(1,(int)std::lround(bm.bmWidth*scale)),dh=std::max(1,(int)std::lround(bm.bmHeight*scale)); int x=thumb.left+((thumb.right-thumb.left)-dw)/2,y=thumb.top+((thumb.bottom-thumb.top)-dh)/2; SetStretchBltMode(d->hDC,HALFTONE); StretchBlt(d->hDC,x,y,dw,dh,mem,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY); SelectObject(mem,old); DeleteDC(mem); } } DeleteObject(hb); }
            RECT text={d->rcItem.left+48,d->rcItem.top+2,d->rcItem.right-5,d->rcItem.bottom-2}; SetBkMode(d->hDC,TRANSPARENT); SetTextColor(d->hDC,fg); DrawTextW(d->hDC,g_brushesKodanima[d->itemID].name.c_str(),-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            if(d->itemState&ODS_FOCUS) DrawFocusRect(d->hDC,&d->rcItem); return TRUE;
        }
        if(d&&d->CtlID==ID_BRUSH_DELETE){HBRUSH b=CreateSolidBrush(RGB(190,42,54));FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,RGB(255,255,255));DrawTextW(d->hDC,L"X Excluir",-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return TRUE;}
    }
    if(msg==WM_PAINT){
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); RECT rc{265,38,495,303};
        HBRUSH bg=CreateSolidBrush(RGB(28,30,36)); FillRect(dc,&rc,bg); DeleteObject(bg);
        // Borda e fundo próprios do quadro de prévia.
        HPEN pen=CreatePen(PS_SOLID,1,RGB(86,90,101)); HGDIOBJ oldPen=SelectObject(dc,pen); HGDIOBJ oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); Rectangle(dc,rc.left,rc.top,rc.right,rc.bottom); SelectObject(dc,oldBrush); SelectObject(dc,oldPen); DeleteObject(pen);
        int ix=(int)SendMessageW(GetDlgItem(hwnd,ID_BRUSH_LIST),LB_GETCURSEL,0,0);
        if(ix>=0&&ix<(int)g_brushesKodanima.size()) {
            if(g_brushSelecionado!=ix || g_activeBrushMask.empty()) AtivarBrushSelecionado(ix);
            int iw=g_activeBrushWidth, ih=g_activeBrushHeight;
            if(iw>0&&ih>0&&!g_activeBrushMask.empty()) {
                const int maxW=210,maxH=210;
                double sc=std::min((double)maxW/(double)iw,(double)maxH/(double)ih);
                int dw=std::max(1,(int)std::lround(iw*sc)),dh=std::max(1,(int)std::lround(ih*sc));
                int px=rc.left+(rc.right-rc.left-dw)/2, py=rc.top+(rc.bottom-rc.top-dh)/2;
                BITMAPINFO bi{}; bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth=iw; bi.bmiHeader.biHeight=-ih; bi.bmiHeader.biPlanes=1; bi.bmiHeader.biBitCount=32; bi.bmiHeader.biCompression=BI_RGB;
                std::vector<DWORD> pixels((size_t)iw*ih,0);
                for(size_t pi=0;pi<pixels.size();++pi){ DWORD v=g_activeBrushMask[pi]; pixels[pi]=(v<<16)|(v<<8)|v; }
                SetStretchBltMode(dc,HALFTONE);
                StretchDIBits(dc,px,py,dw,dh,0,0,iw,ih,pixels.data(),&bi,DIB_RGB_COLORS,SRCCOPY);
                SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(185,190,200));
                RECT nameRect{rc.left+4,rc.bottom-23,rc.right-4,rc.bottom-4};
                DrawTextW(dc,g_brushesKodanima[ix].name.c_str(),-1,&nameRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            } else {
                SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(225,130,125));
                { std::wstring msg=KodaTraduzirTexto(L"Não foi possível carregar a máscara do pincel."); DrawTextW(dc,msg.c_str(),-1,&rc,DT_CENTER|DT_VCENTER|DT_WORDBREAK); }
            }
        } else {
            SetTextColor(dc,RGB(190,195,205)); SetBkMode(dc,TRANSPARENT);
            { std::wstring msg=KodaTraduzirTexto(L"Nenhum pincel carregado.\nClique em + para importar um BMP/PNG.\nArquivos BMP válidos em AppData\\Roaming\\Pinceis do Kodanima também aparecem aqui."); DrawTextW(dc,msg.c_str(),-1,&rc,DT_CENTER|DT_VCENTER|DT_WORDBREAK); }
        }
        EndPaint(hwnd,&ps); return 0;
    }
    if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;} if(msg==WM_DESTROY){if(hwndBrushManager==hwnd)hwndBrushManager=NULL;return 0;} return DefWindowProcW(hwnd,msg,wParam,lParam);
}


static void SetEditFloatBrush(HWND parent,int id,float v){ wchar_t b[48]; swprintf(b,48,L"%.2f",(double)v); SetWindowTextW(GetDlgItem(parent,id),b); }
static float GetEditFloatBrush(HWND parent,int id,float fallback,float lo,float hi){ wchar_t b[64]=L""; HWND e=GetDlgItem(parent,id); if(!e)return fallback; GetWindowTextW(e,b,64); for(wchar_t* q=b;*q;++q)if(*q==L',')*q=L'.'; if(!b[0])return fallback; return (float)std::max((double)lo,std::min((double)hi,wcstod(b,NULL))); }
static void AddBrushPropEdit(HWND h,const wchar_t* label,int id,int x,int y,float value){ CreateWindowW(L"STATIC",label,WS_VISIBLE|WS_CHILD,x,y,190,21,h,NULL,hInstGlobal,NULL); HWND e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,x+190,y-2,100,23,h,(HMENU)(INT_PTR)id,hInstGlobal,NULL); SetEditFloatBrush(h,id,value); }
static void AddBrushPropCheck(HWND h,const wchar_t* label,int id,int x,int y,bool checked){ HWND c=CreateWindowW(L"BUTTON",label,WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,x,y,360,23,h,(HMENU)(INT_PTR)id,hInstGlobal,NULL); SendMessageW(c,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0); }
static void EscolherTexturaExtraBrush(HWND h,int slot){ int ix=g_brushSelecionado; if(ix<0||ix>=(int)g_brushesKodanima.size())return; wchar_t selected[MAX_PATH]={0}; OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=h;ofn.lpstrFile=selected;ofn.nMaxFile=MAX_PATH;ofn.lpstrFilter=L"Texturas BMP/PNG\0*.bmp;*.png\0Todos os arquivos\0*.*\0\0";ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_EXPLORER;const std::wstring title=KodaTraduzirTexto(slot==2?L"Escolher segunda textura":L"Escolher terceira textura");ofn.lpstrTitle=title.c_str();if(!GetOpenFileNameW(&ofn))return;std::wstring folder; if(!PastaTexturasBrushes(folder)){MessageBoxW(h,g_brushStorageError.c_str(),L"Texturas",MB_OK|MB_ICONERROR);return;}std::wstring dest; if(!ImportarImagemRasterizada(selected,folder,slot==2?L"Textura2_":L"Textura3_",false,NULL,dest)){MessageBoxW(h,g_brushStorageError.c_str(),L"Texturas",MB_OK|MB_ICONERROR);return;}if(slot==2)g_brushesKodanima[ix].texture2Path=dest;else g_brushesKodanima[ix].texture3Path=dest;if(!SalvarBibliotecaBrushesJson(g_brushesKodanima)){MessageBoxW(h,g_brushStorageError.c_str(),L"Texturas",MB_OK|MB_ICONERROR);return;}AtivarBrushSelecionado(ix);InvalidateRect(hwndCanvas,NULL,FALSE);if(hwndBrushManager&&IsWindow(hwndBrushManager))InvalidateRect(hwndBrushManager,NULL,TRUE); }
static void OpenBrushPage(HWND owner,int page);
static bool BrushHasControl(HWND h,int id){return GetDlgItem(h,id)!=NULL;}
static void SetBrushCheckFromControl(HWND h,int id,int& value){if(BrushHasControl(h,id))value=SendMessageW(GetDlgItem(h,id),BM_GETCHECK,0,0)==BST_CHECKED?1:0;}
static void SetBrushNumberFromControl(HWND h,int id,float& value,float lo,float hi){if(BrushHasControl(h,id))value=GetEditFloatBrush(h,id,value,lo,hi);}
static LRESULT CALLBACK WndProcBrushProperties(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m==WM_COMMAND) {
        const int id=LOWORD(w);
        if(id>=ID_BRUSH_PAGE_TIP && id<=ID_BRUSH_PAGE_MKV) {
            HWND owner=g_brushPropertiesOwner;
            g_brushPropertiesReturnPage=g_brushPropertiesPage;
            DestroyWindow(h);
            OpenBrushPage(owner,id-ID_BRUSH_PAGE_TIP+1);
            return 0;
        }
        if(id==ID_BRUSH_PROP_CANCEL){DestroyWindow(h);return 0;}
        if(id==ID_BRUSH_PAGE_BACK){
            HWND owner=g_brushPropertiesOwner;
            const int backPage=(g_brushPropertiesPage==8 && g_brushPropertiesReturnPage==8)?0:g_brushPropertiesReturnPage;
            DestroyWindow(h);
            OpenBrushPage(owner,backPage);
            return 0;
        }
        if(id==ID_BRUSH_PROP_TEXTURE2){EscolherTexturaExtraBrush(h,2);return 0;}
        if(id==ID_BRUSH_PROP_TEXTURE3){EscolherTexturaExtraBrush(h,3);return 0;}
        if(id==ID_BRUSH_PROP_TINT2||id==ID_BRUSH_PROP_TINT3){
            int ix=g_brushSelecionado;if(ix<0||ix>=(int)g_brushesKodanima.size())return 0;
            static COLORREF custom[16]={0};CHOOSECOLORW cc{};cc.lStructSize=sizeof(cc);cc.hwndOwner=h;cc.lpCustColors=custom;cc.rgbResult=id==ID_BRUSH_PROP_TINT2?g_brushesKodanima[ix].texture2Tint:g_brushesKodanima[ix].texture3Tint;cc.Flags=CC_FULLOPEN|CC_RGBINIT|CC_ENABLEHOOK;cc.lpfnHook=KodaHookSeletorCor;
            if(ChooseColorW(&cc)){if(id==ID_BRUSH_PROP_TINT2)g_brushesKodanima[ix].texture2Tint=cc.rgbResult;else g_brushesKodanima[ix].texture3Tint=cc.rgbResult;if(!SalvarBibliotecaBrushesJson(g_brushesKodanima))MessageBoxW(h,g_brushStorageError.c_str(),L"Salvar textura",MB_OK|MB_ICONWARNING);AtivarBrushSelecionado(ix);InvalidateRect(hwndCanvas,NULL,FALSE);}
            return 0;
        }
        if(id==ID_BRUSH_PROP_SAVE){
            int ix=g_brushSelecionado;if(ix<0||ix>=(int)g_brushesKodanima.size())return 0;
            BrushEntryKodanima& e=g_brushesKodanima[ix];BrushMetaKodanima& b=e.meta;
            if(BrushHasControl(h,ID_BRUSH_PROP_NAME)){wchar_t nm[256]=L"";GetWindowTextW(GetDlgItem(h,ID_BRUSH_PROP_NAME),nm,256);e.name=NomeArquivoSeguroBrush(nm);}
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SIZE,b.size,0.1f,20000.0f);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_MIN,b.minSize,0.01f,5000.0f);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_MAX,b.maxSize,b.minSize,5000.0f);
            b.maxSize=std::max(b.minSize,b.maxSize);
            if(BrushHasControl(h,ID_BRUSH_PROP_HARDNESS))b.hardness=(int)GetEditFloatBrush(h,ID_BRUSH_PROP_HARDNESS,(float)b.hardness,0,3);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_ROUNDNESS,b.roundnessPercent,1,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SPACING,b.spacingPercent,1,500);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_ANGLE,b.angleDegrees,-180,180);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SCATTER,b.scatterPercent,0,500);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_OPACITY,b.opacityPercent,0,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_FLOW,b.flowPercent,1,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_STARTSIZE,b.startSizePercent,1,500);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_ENDSIZE,b.endSizePercent,1,500);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_STARTOPACITY,b.startOpacityPercent,0,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_ENDOPACITY,b.endOpacityPercent,0,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SPRAYDENSITY,b.sprayDensity,1,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SPRAYRADIUS,b.sprayRadiusPercent,0,500);
            if(BrushHasControl(h,ID_BRUSH_PROP_PARTICLES))b.particleCount=(int)GetEditFloatBrush(h,ID_BRUSH_PROP_PARTICLES,(float)b.particleCount,1,32);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SPREAD,b.particleSpreadPercent,0,500);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_SCALERAND,b.scaleRandomPercent,0,100);
            SetBrushNumberFromControl(h,ID_BRUSH_PROP_BLUR,b.blurPercent,0,100);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_ROTATE,b.rotateWithStroke);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_RANDOMROT,b.randomRotation);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_SPRAY,b.sprayingEffect);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_PARTICLESON,b.particlesEnabled);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_FOLLOW,b.followPenDirection);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_ANTIOVERFLOW,b.antiOverflow);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_OPTIMIZE,b.optimizeStroke);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_SIZEPRESS,b.sizeByPressure);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_OPACITYPRESS,b.opacityByPressure);
            SetBrushCheckFromControl(h,ID_BRUSH_PROP_STARTEND,b.startEndDynamics);
            if(BrushHasControl(h,ID_BRUSH_PROP_ANTIALIAS))b.antialias=SendMessageW(GetDlgItem(h,ID_BRUSH_PROP_ANTIALIAS),BM_GETCHECK,0,0)==BST_CHECKED?1:0;
            if(!SalvarBibliotecaBrushesJson(g_brushesKodanima)){MessageBoxW(h,g_brushStorageError.c_str(),L"Salvar configurações do pincel",MB_OK|MB_ICONERROR);return 0;}
            AtivarBrushSelecionado(ix);
            g_brushOptimizedStroke=(g_brushesKodanima[ix].meta.optimizeStroke!=0);
            if(hwndBrushManager&&IsWindow(hwndBrushManager)){RecarregarListaBrushes(hwndBrushManager);SendMessageW(GetDlgItem(hwndBrushManager,ID_BRUSH_LIST),LB_SETCURSEL,ix,0);SetWindowTextW(GetDlgItem(hwndBrushManager,ID_BRUSH_NAME_EDIT),g_brushesKodanima[ix].name.c_str());SendMessageW(GetDlgItem(hwndBrushManager,ID_BRUSH_USE_IMPORTED),BM_SETCHECK,g_useImportedBrushes?BST_CHECKED:BST_UNCHECKED,0);InvalidateRect(hwndBrushManager,NULL,TRUE);}
            HWND owner=g_brushPropertiesOwner;
            const int returnPage=g_brushPropertiesPage;
            // Permanece na página salva para que a caixa (incluindo Otimizar traço)
            // continue visível e possa ser marcada/desmarcada sem procurar novamente.
            DestroyWindow(h);
            if(returnPage!=0) OpenBrushPage(owner,returnPage);
            if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE);
            return 0;
        }
    }
    if(m==WM_CLOSE){DestroyWindow(h);return 0;}
    if(m==WM_DESTROY){if(hwndBrushProperties==h)hwndBrushProperties=NULL;return 0;}
    return DefWindowProcW(h,m,w,l);
}
static void AddBrushPageButton(HWND h,const wchar_t* label,int id,int x,int y,int w,int ht){CreateWindowW(L"BUTTON",label,WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,x,y,w,ht,h,(HMENU)(INT_PTR)id,hInstGlobal,NULL);}
static const wchar_t* BrushPageTitle(int page){switch(page){case 0:return L"Sub Tool Detail — Configurações do pincel";case 1:return L"Brush Tip — Ponta do pincel";case 2:return L"Stroke — Traçado";case 3:return L"Texture — Texturas do pincel";case 4:return L"Spraying Effect — Pulverização";case 5:return L"Starting and Ending — Início e fim";case 6:return L"Ink / Pressure — Tinta e pressão";case 7:return L"Anti-overflow e desempenho";default:return L"Mecânicas do brush (MKV)";}}
static void OpenBrushPage(HWND owner,int page){
    if(g_brushSelecionado<0||g_brushSelecionado>=(int)g_brushesKodanima.size()){MessageBoxW(owner,L"Selecione um pincel importado para editar as configurações.",L"Propriedades do pincel",MB_OK|MB_ICONINFORMATION);return;}
    if(hwndBrushProperties&&IsWindow(hwndBrushProperties))DestroyWindow(hwndBrushProperties);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProcBrushProperties;wc.hInstance=hInstGlobal;wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));wc.hIcon=hIconeApp;wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"KodanimaBrushPropertiesClass";RegisterClassExW(&wc);
    g_brushPropertiesPage=page;g_brushPropertiesOwner=owner;
    RECT wa{};SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0);int width=page==0?610:(page==8?720:640),height=page==0?420:(page==8?690:540);int x=(int)(wa.left+(wa.right-wa.left-width)/2),y=(int)(wa.top+(wa.bottom-wa.top-height)/2);if(x<wa.left)x=wa.left;if(y<wa.top)y=wa.top;
    hwndBrushProperties=CreateWindowExW(WS_EX_TOOLWINDOW,L"KodanimaBrushPropertiesClass",BrushPageTitle(page),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_VSCROLL,x,y,width,height,owner,NULL,hInstGlobal,NULL);
    if(!hwndBrushProperties)return;HWND h=hwndBrushProperties;const BrushMetaKodanima& b=g_brushesKodanima[g_brushSelecionado].meta;
    if(page==0){
        CreateWindowW(L"STATIC",L"Configurações clássicas em janelas separadas; MKV contém as mecânicas próprias e atalhos para todos estes painéis.",WS_VISIBLE|WS_CHILD,16,12,550,34,h,NULL,hInstGlobal,NULL);
        AddBrushPageButton(h,L"Brush Tip — forma e tamanho",ID_BRUSH_PAGE_TIP,18,58,270,38);
        AddBrushPageButton(h,L"Stroke — espaçamento e fluxo",ID_BRUSH_PAGE_STROKE,310,58,270,38);
        AddBrushPageButton(h,L"Texture — camadas de textura",ID_BRUSH_PAGE_TEXTURE,18,108,270,38);
        AddBrushPageButton(h,L"Spraying Effect — pulverização",ID_BRUSH_PAGE_SPRAY,310,108,270,38);
        AddBrushPageButton(h,L"Starting and Ending",ID_BRUSH_PAGE_STARTEND,18,158,270,38);
        AddBrushPageButton(h,L"Ink / Pressure — pressão",ID_BRUSH_PAGE_PRESSURE,310,158,270,38);
        AddBrushPageButton(h,L"Anti-overflow e desempenho",ID_BRUSH_PAGE_SAFETY,18,208,270,38);
        AddBrushPageButton(h,L"Mecânicas próprias (MKV)",ID_BRUSH_PAGE_MKV,310,208,270,38);
        CreateWindowW(L"STATIC",L"MKV reúne rotação, escala aleatória, partículas, dispersão e blur. As categorias à esquerda seguem a organização inspirada em Sub Tool Detail.",WS_VISIBLE|WS_CHILD,18,265,560,65,h,NULL,hInstGlobal,NULL);
        AddBrushPageButton(h,L"Fechar",ID_BRUSH_PROP_CANCEL,475,345,105,30);
    } else {
        if(page==1){
            AddBrushPropEdit(h,L"Tamanho",ID_BRUSH_PROP_SIZE,20,52,b.size);AddBrushPropEdit(h,L"Tamanho mínimo",ID_BRUSH_PROP_MIN,20,88,b.minSize);AddBrushPropEdit(h,L"Tamanho máximo",ID_BRUSH_PROP_MAX,20,124,b.maxSize);AddBrushPropEdit(h,L"Dureza (0–3)",ID_BRUSH_PROP_HARDNESS,20,160,(float)b.hardness);AddBrushPropEdit(h,L"Redondez (%)",ID_BRUSH_PROP_ROUNDNESS,20,196,b.roundnessPercent);AddBrushPropCheck(h,L"Antialiasing da ponta",ID_BRUSH_PROP_ANTIALIAS,20,238,b.antialias!=0);
        } else if(page==2){
            AddBrushPropEdit(h,L"Espaçamento (%)",ID_BRUSH_PROP_SPACING,20,52,b.spacingPercent);AddBrushPropEdit(h,L"Opacidade (%)",ID_BRUSH_PROP_OPACITY,20,88,b.opacityPercent);AddBrushPropEdit(h,L"Fluxo (%)",ID_BRUSH_PROP_FLOW,20,124,b.flowPercent);AddBrushPropEdit(h,L"Dispersão básica (%)",ID_BRUSH_PROP_SCATTER,20,160,b.scatterPercent);CreateWindowW(L"STATIC",L"O espaçamento define a distância entre as pontas repetidas ao longo do traço.",WS_VISIBLE|WS_CHILD,20,215,520,36,h,NULL,hInstGlobal,NULL);
        } else if(page==3){
            CreateWindowW(L"STATIC",L"Camada de textura 2 — imagem BMP/PNG",WS_VISIBLE|WS_CHILD,20,52,290,22,h,NULL,hInstGlobal,NULL);AddBrushPageButton(h,L"Escolher textura 2...",ID_BRUSH_PROP_TEXTURE2,20,80,180,30);AddBrushPageButton(h,L"Tom da textura 2...",ID_BRUSH_PROP_TINT2,215,80,180,30);
            CreateWindowW(L"STATIC",L"Camada de textura 3 — imagem BMP/PNG",WS_VISIBLE|WS_CHILD,20,142,290,22,h,NULL,hInstGlobal,NULL);AddBrushPageButton(h,L"Escolher textura 3...",ID_BRUSH_PROP_TEXTURE3,20,170,180,30);AddBrushPageButton(h,L"Tom da textura 3...",ID_BRUSH_PROP_TINT3,215,170,180,30);CreateWindowW(L"STATIC",L"As camadas e tonalidades ficam vinculadas a este pincel no JSON mestre.",WS_VISIBLE|WS_CHILD,20,230,550,30,h,NULL,hInstGlobal,NULL);
        } else if(page==4){
            AddBrushPropCheck(h,L"Ativar Spraying Effect",ID_BRUSH_PROP_SPRAY,20,52,b.sprayingEffect!=0);AddBrushPropEdit(h,L"Densidade do spray",ID_BRUSH_PROP_SPRAYDENSITY,20,94,b.sprayDensity);AddBrushPropEdit(h,L"Raio do spray (%)",ID_BRUSH_PROP_SPRAYRADIUS,20,130,b.sprayRadiusPercent);CreateWindowW(L"STATIC",L"A pulverização replica a ponta em posições pseudorrandômicas em torno do traço.",WS_VISIBLE|WS_CHILD,20,190,540,38,h,NULL,hInstGlobal,NULL);
        } else if(page==5){
            AddBrushPropCheck(h,L"Ativar Starting / Ending",ID_BRUSH_PROP_STARTEND,20,52,b.startEndDynamics!=0);AddBrushPropEdit(h,L"Tamanho inicial (%)",ID_BRUSH_PROP_STARTSIZE,20,94,b.startSizePercent);AddBrushPropEdit(h,L"Tamanho final (%)",ID_BRUSH_PROP_ENDSIZE,20,130,b.endSizePercent);AddBrushPropEdit(h,L"Opacidade inicial (%)",ID_BRUSH_PROP_STARTOPACITY,20,166,b.startOpacityPercent);AddBrushPropEdit(h,L"Opacidade final (%)",ID_BRUSH_PROP_ENDOPACITY,20,202,b.endOpacityPercent);
        } else if(page==6){
            AddBrushPropCheck(h,L"Tamanho controlado pela pressão",ID_BRUSH_PROP_SIZEPRESS,20,52,b.sizeByPressure!=0);AddBrushPropCheck(h,L"Opacidade controlada pela pressão",ID_BRUSH_PROP_OPACITYPRESS,20,88,b.opacityByPressure!=0);AddBrushPropEdit(h,L"Tamanho mínimo (pressão baixa)",ID_BRUSH_PROP_MIN,20,132,b.minSize);AddBrushPropEdit(h,L"Tamanho máximo (pressão alta)",ID_BRUSH_PROP_MAX,20,168,b.maxSize);CreateWindowW(L"STATIC",L"A pressão depende dos eventos recebidos da caneta/tablet; mouse normalmente fornece pressão binária.",WS_VISIBLE|WS_CHILD,20,225,555,42,h,NULL,hInstGlobal,NULL);
        } else if(page==7){
            AddBrushPropCheck(h,L"Anti-overflow — limitar ao recorte válido",ID_BRUSH_PROP_ANTIOVERFLOW,20,52,b.antiOverflow!=0);AddBrushPropCheck(h,L"Otimizar traço — aplicar ao soltar",ID_BRUSH_PROP_OPTIMIZE,20,88,b.optimizeStroke!=0);AddBrushPropCheck(h,L"Antialiasing da ponta",ID_BRUSH_PROP_ANTIALIAS,20,124,b.antialias!=0);CreateWindowW(L"STATIC",L"Otimizar traço guarda segmentos durante a entrada e aplica o pincel ao terminar. Anti-overflow respeita a região de desenho.",WS_VISIBLE|WS_CHILD,20,180,555,45,h,NULL,hInstGlobal,NULL);
        } else { // MKV: recursos próprios do motor do Kodanima.
            AddBrushPropCheck(h,L"Rotacionar acompanhando o traço",ID_BRUSH_PROP_ROTATE,20,48,b.rotateWithStroke!=0);AddBrushPropCheck(h,L"Rotação aleatória por carimbo",ID_BRUSH_PROP_RANDOMROT,20,78,b.randomRotation!=0);AddBrushPropCheck(h,L"Seguir direção da caneta",ID_BRUSH_PROP_FOLLOW,20,108,b.followPenDirection!=0);AddBrushPropCheck(h,L"Ativar partículas",ID_BRUSH_PROP_PARTICLESON,20,138,b.particlesEnabled!=0);AddBrushPropCheck(h,L"Ativar Spraying Effect",ID_BRUSH_PROP_SPRAY,20,168,b.sprayingEffect!=0);
            AddBrushPropEdit(h,L"Ângulo base (graus)",ID_BRUSH_PROP_ANGLE,20,215,b.angleDegrees);AddBrushPropEdit(h,L"Escala aleatória (%)",ID_BRUSH_PROP_SCALERAND,20,251,b.scaleRandomPercent);AddBrushPropEdit(h,L"Blur da ponta (%)",ID_BRUSH_PROP_BLUR,20,287,b.blurPercent);AddBrushPropEdit(h,L"Partículas por passo",ID_BRUSH_PROP_PARTICLES,330,215,(float)b.particleCount);AddBrushPropEdit(h,L"Dispersão de partículas (%)",ID_BRUSH_PROP_SPREAD,330,251,b.particleSpreadPercent);AddBrushPropEdit(h,L"Densidade do spray",ID_BRUSH_PROP_SPRAYDENSITY,330,287,b.sprayDensity);AddBrushPropEdit(h,L"Raio do spray (%)",ID_BRUSH_PROP_SPRAYRADIUS,330,323,b.sprayRadiusPercent);AddBrushPropEdit(h,L"Dispersão base (%)",ID_BRUSH_PROP_SCATTER,330,359,b.scatterPercent);
            CreateWindowW(L"STATIC",L"Blur suaviza a máscara 3x3; escala e rotação variam de modo determinístico. Os botões abaixo abrem cada painel Sub Tool Detail sem perder as configurações MKV.",WS_VISIBLE|WS_CHILD,20,400,660,36,h,NULL,hInstGlobal,NULL);
            CreateWindowW(L"STATIC",L"Acesso rápido a todas as configurações Sub Tool Detail:",WS_VISIBLE|WS_CHILD,16,440,660,20,h,NULL,hInstGlobal,NULL);
            AddBrushPageButton(h,L"Brush Tip",ID_BRUSH_PAGE_TIP,16,463,90,28);
            AddBrushPageButton(h,L"Stroke",ID_BRUSH_PAGE_STROKE,111,463,90,28);
            AddBrushPageButton(h,L"Texture",ID_BRUSH_PAGE_TEXTURE,206,463,90,28);
            AddBrushPageButton(h,L"Spray",ID_BRUSH_PAGE_SPRAY,301,463,90,28);
            AddBrushPageButton(h,L"Start/End",ID_BRUSH_PAGE_STARTEND,396,463,90,28);
            AddBrushPageButton(h,L"Ink/Pressão",ID_BRUSH_PAGE_PRESSURE,491,463,90,28);
            AddBrushPageButton(h,L"Anti-overflow",ID_BRUSH_PAGE_SAFETY,586,463,96,28);
        }
        const int saveY=(page==8)?625:445;
        AddBrushPageButton(h,L"Salvar",ID_BRUSH_PROP_SAVE,(page==8?395:330),saveY,105,30);
        AddBrushPageButton(h,L"Voltar às categorias",ID_BRUSH_PAGE_BACK,(page==8?505:442),saveY,(page==8?175:150),30);
        AddBrushPageButton(h,L"Cancelar",ID_BRUSH_PROP_CANCEL,20,saveY,100,30);
    }
    ShowWindow(h,SW_SHOWNORMAL);UpdateWindow(h);
}
static void AbrirPropriedadesBrush(HWND owner,bool mechanics){
    g_brushPropertiesReturnPage=0;
    OpenBrushPage(owner,mechanics?8:0);
}


static int ferramentaPropriedadesAtual = ID_TOOL_LAPIS;
void AbrirPreferenciasFerramenta(HWND owner, int toolId) {
    if(toolId!=-1) ferramentaPropriedadesAtual=toolId;
    if (hwndToolSettings && IsWindow(hwndToolSettings)) { DestroyWindow(hwndToolSettings); hwndToolSettings=NULL; }
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=WndProcToolSettings; wc.hInstance=hInstGlobal;
    wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512)); wc.hIcon=hIconeApp; wc.hIconSm=hIconeApp;
    wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1); wc.lpszClassName=L"KodanimaToolSettingsClass";
    RegisterClassExW(&wc);
    POINT pt{}; GetCursorPos(&pt); toolSettingsMouseEntered=false;
    // Abre o painel ao lado da barra de ferramentas, não embaixo do cursor.
    // Isso evita que o primeiro clique acerte o título da janela e que o painel
    // desapareça quando o usuário move o mouse para alcançar os controles.
    RECT toolRect{}; if(hwndTools && IsWindow(hwndTools) && GetWindowRect(hwndTools,&toolRect)) { pt.x=toolRect.right+8; pt.y=toolRect.top; }
    const wchar_t* titulo=L"Propriedades do lápis"; int altura=485;
    if(ferramentaPropriedadesAtual==ID_TOOL_BORRACHA){titulo=L"Propriedades da borracha";altura=210;}
    else if(ferramentaPropriedadesAtual==ID_TOOL_BALDE){titulo=L"Propriedades do balde";altura=155;}
    else if(ferramentaPropriedadesAtual==ID_TOOL_LACON){titulo=L"Propriedades do laço";altura=190;}
    hwndToolSettings=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,L"KodanimaToolSettingsClass",titulo,
        WS_POPUP|WS_CAPTION|WS_BORDER,pt.x,pt.y,350,altura,owner,NULL,hInstGlobal,NULL);
    if(ferramentaPropriedadesAtual==ID_TOOL_LAPIS){
        CreateWindowW(L"STATIC",L"Grossura do lápis:",WS_VISIBLE|WS_CHILD,10,12,180,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        HWND e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,220,10,95,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_SIZE,hInstGlobal,NULL); { wchar_t v[48]; swprintf(v,48,L"%.3f",(double)tamanhoLapis); SetWindowTextW(e,v); }
        HWND c=CreateWindowW(L"BUTTON",L"Usar pressão da caneta",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,45,230,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_PRESS,hInstGlobal,NULL); SendMessageW(c,BM_SETCHECK,pressaoCanetaAtiva?BST_CHECKED:BST_UNCHECKED,0);
        CreateWindowW(L"STATIC",L"Espessura mínima:",WS_VISIBLE|WS_CHILD,10,78,180,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,220,76,95,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_MINPRESS,hInstGlobal,NULL); { wchar_t v[48]; swprintf(v,48,L"%.3f",(double)tamanhoMinimoPressao); SetWindowTextW(e,v); }
        CreateWindowW(L"STATIC",L"Espessura máxima:",WS_VISIBLE|WS_CHILD,10,110,180,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,220,108,95,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_MAXPRESS,hInstGlobal,NULL); { wchar_t v[48]; swprintf(v,48,L"%.3f",(double)tamanhoMaximoPressao); SetWindowTextW(e,v); }
        HWND aa=CreateWindowW(L"BUTTON",L"Suavizar bordas (antialiasing)",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,142,250,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ANTIALIAS,hInstGlobal,NULL); SendMessageW(aa,BM_SETCHECK,lapisAntiAlias?BST_CHECKED:BST_UNCHECKED,0);
        HWND stab=CreateWindowW(L"BUTTON",L"Estabilizador do traço",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,170,145,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_STABILIZER_ENABLE,hInstGlobal,NULL); SendMessageW(stab,BM_SETCHECK,estabilizadorTracoAtivo?BST_CHECKED:BST_UNCHECKED,0);
        CreateWindowW(L"STATIC",L"Força (0-100):",WS_VISIBLE|WS_CHILD,157,172,94,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,252,168,62,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_STABILIZER_LEVEL,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(nivelEstabilizadorTraco).c_str());
        CreateWindowW(L"STATIC",L"Dureza do pincel:",WS_VISIBLE|WS_CHILD,10,200,150,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        const int hardIds[4]={ID_TOOLSETTINGS_HARDNESS1,ID_TOOLSETTINGS_HARDNESS2,ID_TOOLSETTINGS_HARDNESS3,ID_TOOLSETTINGS_HARDNESS4};
        const wchar_t* hardNames[4]={L"○ Macio",L"◔",L"◑",L"● Duro"};
        const wchar_t* hardLabels[4]={L"Macio",L"Suave",L"Firme",L"Duro"};
        for(int hi=0;hi<4;++hi){ CreateWindowW(L"BUTTON",hardLabels[hi],WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,10+hi*78,225,74,38,hwndToolSettings,(HMENU)hardIds[hi],hInstGlobal,NULL); }
        CreateWindowW(L"STATIC",L"Espaçamento %:",WS_VISIBLE|WS_CHILD,10,270,125,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,135,267,65,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_SPACING,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushSpacingPercent);SetWindowTextW(e,v);}
        CreateWindowW(L"STATIC",L"Ângulo °:",WS_VISIBLE|WS_CHILD,210,270,70,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,278,267,48,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ANGLE,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushAngleDegrees);SetWindowTextW(e,v);}
        CreateWindowW(L"STATIC",L"Redondez %:",WS_VISIBLE|WS_CHILD,10,301,125,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,135,298,65,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ROUNDNESS,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushRoundnessPercent);SetWindowTextW(e,v);}
        CreateWindowW(L"STATIC",L"Dispersão %:",WS_VISIBLE|WS_CHILD,210,301,70,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,278,298,48,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_SCATTER,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushScatterPercent);SetWindowTextW(e,v);}
        CreateWindowW(L"STATIC",L"Opacidade %:",WS_VISIBLE|WS_CHILD,10,332,125,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,135,329,65,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_OPACITY,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushOpacityPercent);SetWindowTextW(e,v);}
        CreateWindowW(L"STATIC",L"Fluxo %:",WS_VISIBLE|WS_CHILD,210,332,70,20,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,278,329,48,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_FLOW,hInstGlobal,NULL); {wchar_t v[32];swprintf(v,32,L"%.1f",(double)brushFlowPercent);SetWindowTextW(e,v);}
        HWND rot=CreateWindowW(L"BUTTON",L"Girar com direção do traço",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,361,190,22,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ROTATE_STROKE,hInstGlobal,NULL); SendMessageW(rot,BM_SETCHECK,brushRotateWithStroke?BST_CHECKED:BST_UNCHECKED,0);
        HWND szp=CreateWindowW(L"BUTTON",L"Tamanho por pressão",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,384,155,22,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_SIZE_DYNAMICS,hInstGlobal,NULL); SendMessageW(szp,BM_SETCHECK,brushSizeByPressure?BST_CHECKED:BST_UNCHECKED,0);
        HWND op=CreateWindowW(L"BUTTON",L"Opacidade por pressão",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,175,384,155,22,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_OPACITY_DYNAMICS,hInstGlobal,NULL); SendMessageW(op,BM_SETCHECK,brushOpacityByPressure?BST_CHECKED:BST_UNCHECKED,0);
        CreateWindowW(L"BUTTON",L"Salvar e fechar",WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,105,418,140,28,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_OK,hInstGlobal,NULL);
        CreateWindowW(L"BUTTON",L"Cancelar",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,250,418,75,28,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_CLOSE,hInstGlobal,NULL);
    } else if(ferramentaPropriedadesAtual==ID_TOOL_BORRACHA){
        CreateWindowW(L"STATIC",L"Grossura da borracha:",WS_VISIBLE|WS_CHILD,10,12,190,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        HWND e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,220,10,75,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ERASER_SIZE,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(tamanhoBorracha).c_str());
        HWND c=CreateWindowW(L"BUTTON",L"Usar pressão na borracha",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,10,45,240,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ERASER_PRESS,hInstGlobal,NULL); SendMessageW(c,BM_SETCHECK,pressaoBorrachaAtiva?BST_CHECKED:BST_UNCHECKED,0);
        CreateWindowW(L"STATIC",L"Espessura mínima:",WS_VISIBLE|WS_CHILD,10,78,180,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,220,76,75,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ERASER_MIN,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(pressaoMinBorracha).c_str());
        CreateWindowW(L"STATIC",L"Espessura máxima:",WS_VISIBLE|WS_CHILD,10,110,180,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,220,108,75,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_ERASER_MAX,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(pressaoMaxBorracha).c_str());
    } else if(ferramentaPropriedadesAtual==ID_TOOL_BALDE){
        CreateWindowW(L"STATIC",L"Gap do balde (0-100):",WS_VISIBLE|WS_CHILD,10,12,190,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        HWND e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,220,10,75,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_GAP,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(nivelGapBalde).c_str());
        CreateWindowW(L"STATIC",L"Tolerância (0-20 px):",WS_VISIBLE|WS_CHILD,10,45,190,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER,220,43,75,24,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_TOL,hInstGlobal,NULL); SetWindowTextW(e,std::to_wstring(toleranciaBalde).c_str());
    } else if(ferramentaPropriedadesAtual==ID_TOOL_LACON){
        CreateWindowW(L"STATIC",L"Modo de seleção:",WS_VISIBLE|WS_CHILD,10,12,160,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        HWND combo=CreateWindowW(L"COMBOBOX",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST,150,10,175,120,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_LASSO_MODE,hInstGlobal,NULL);
        SendMessageW(combo,CB_ADDSTRING,0,(LPARAM)L"Retangular"); SendMessageW(combo,CB_ADDSTRING,0,(LPARAM)L"Desenhar área livre"); SendMessageW(combo,CB_SETCURSEL,modoLaco,0);
        CreateWindowW(L"BUTTON",L"Escolher cor da seleção...",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,10,48,210,28,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_LASSO_COLOR,hInstGlobal,NULL);
        CreateWindowW(L"STATIC",L"R = rotacionar | Shift = escala | Enter = confirmar",WS_VISIBLE|WS_CHILD,10,86,320,22,hwndToolSettings,NULL,hInstGlobal,NULL);
        CreateWindowW(L"BUTTON",L"X",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,304,4,30,23,hwndToolSettings,(HMENU)ID_TOOLSETTINGS_CLOSE,hInstGlobal,NULL);
    }
    // As propriedades do lápis permanecem abertas até o usuário salvar ou
    // cancelar; não dependem de polling de hover, que fechava o painel cedo.
    if(ferramentaPropriedadesAtual!=ID_TOOL_LACON && ferramentaPropriedadesAtual!=ID_TOOL_LAPIS) SetTimer(hwndToolSettings,1,120,NULL);
    ShowWindow(hwndToolSettings,SW_SHOWNORMAL); SetForegroundWindow(hwndToolSettings); UpdateWindow(hwndToolSettings);
}

void SalvarEFecharPreferenciasFerramenta(HWND hwnd) {
    if(!hwnd||!IsWindow(hwnd))return; wchar_t t[32]=L"1";
    if(ferramentaPropriedadesAtual==ID_TOOL_LAPIS){
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_SIZE),t,32); for(wchar_t* q=t;*q;++q) if(*q==L',') *q=L'.'; tamanhoLapis=(float)std::max(0.01,std::min(20000.0,wcstod(t,NULL)));
        pressaoCanetaAtiva=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_PRESS),BM_GETCHECK,0,0)==BST_CHECKED;
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_MINPRESS),t,32); for(wchar_t* q=t;*q;++q) if(*q==L',') *q=L'.'; tamanhoMinimoPressao=(float)std::max(0.01,std::min(5000.0,wcstod(t,NULL)));
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_MAXPRESS),t,32); for(wchar_t* q=t;*q;++q) if(*q==L',') *q=L'.'; tamanhoMaximoPressao=(float)std::max((double)tamanhoMinimoPressao,std::min(5000.0,wcstod(t,NULL)));
        lapisAntiAlias=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ANTIALIAS),BM_GETCHECK,0,0)==BST_CHECKED;
        estabilizadorTracoAtivo=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_STABILIZER_ENABLE),BM_GETCHECK,0,0)==BST_CHECKED;
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_STABILIZER_LEVEL),t,32); nivelEstabilizadorTraco=std::max(0,std::min(100,_wtoi(t)));
        auto readBrushFloat=[&](int id,float current,float lo,float hi)->float{wchar_t z[48]=L"";GetWindowTextW(GetDlgItem(hwnd,id),z,48);for(wchar_t* q=z;*q;++q)if(*q==L',')*q=L'.';double v=wcstod(z,NULL);if(!z[0])v=current;return (float)std::max((double)lo,std::min((double)hi,v));};
        brushSpacingPercent=readBrushFloat(ID_TOOLSETTINGS_SPACING,brushSpacingPercent,1.0f,500.0f);
        brushAngleDegrees=readBrushFloat(ID_TOOLSETTINGS_ANGLE,brushAngleDegrees,-180.0f,180.0f);
        brushRoundnessPercent=readBrushFloat(ID_TOOLSETTINGS_ROUNDNESS,brushRoundnessPercent,1.0f,100.0f);
        brushScatterPercent=readBrushFloat(ID_TOOLSETTINGS_SCATTER,brushScatterPercent,0.0f,500.0f);
        brushOpacityPercent=readBrushFloat(ID_TOOLSETTINGS_OPACITY,brushOpacityPercent,0.0f,100.0f);
        brushFlowPercent=readBrushFloat(ID_TOOLSETTINGS_FLOW,brushFlowPercent,1.0f,100.0f);
        brushRotateWithStroke=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ROTATE_STROKE),BM_GETCHECK,0,0)==BST_CHECKED;
        brushSizeByPressure=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_SIZE_DYNAMICS),BM_GETCHECK,0,0)==BST_CHECKED;
        brushOpacityByPressure=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_OPACITY_DYNAMICS),BM_GETCHECK,0,0)==BST_CHECKED;

    } else if(ferramentaPropriedadesAtual==ID_TOOL_BORRACHA){
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ERASER_SIZE),t,32); tamanhoBorracha=std::max(1,std::min(100,_wtoi(t)));
        pressaoBorrachaAtiva=SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ERASER_PRESS),BM_GETCHECK,0,0)==BST_CHECKED;
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ERASER_MIN),t,32); pressaoMinBorracha=std::max(1,std::min(100,_wtoi(t)));
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_ERASER_MAX),t,32); pressaoMaxBorracha=std::max(pressaoMinBorracha,std::min(100,_wtoi(t)));
    } else if(ferramentaPropriedadesAtual==ID_TOOL_BALDE){
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_GAP),t,32); nivelGapBalde=std::max(0,std::min(100,_wtoi(t)));
        GetWindowTextW(GetDlgItem(hwnd,ID_TOOLSETTINGS_TOL),t,32); toleranciaBalde=std::max(0,std::min(20,_wtoi(t)));
    } else if(ferramentaPropriedadesAtual==ID_TOOL_LACON){
        int m=(int)SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_LASSO_MODE),CB_GETCURSEL,0,0); modoLaco=(m==1)?1:0;
    }
    SalvarConfiguracoesUsuario();
    if(!g_activeBrushPath.empty()) for(size_t bi=0;bi<g_brushesKodanima.size();++bi) if(g_brushesKodanima[bi].bmpPath==g_activeBrushPath) { if(!GravarMetaBrush(g_brushesKodanima[bi])) MessageBoxW(hwndToolSettings,g_brushStorageError.c_str(),L"Não foi possível salvar os parâmetros do pincel",MB_OK|MB_ICONERROR); }
    KillTimer(hwnd,1); if(hwndToolSettings==hwnd)hwndToolSettings=NULL; DestroyWindow(hwnd);
    InvalidarCacheComposicaoCanvas(); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE);
}

LRESULT CALLBACK WndProcToolSettings(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if(msg==WM_DRAWITEM){ DRAWITEMSTRUCT* d=(DRAWITEMSTRUCT*)lParam; if(d && d->CtlID>=ID_TOOLSETTINGS_HARDNESS1 && d->CtlID<=ID_TOOLSETTINGS_HARDNESS4){ int ix=(int)d->CtlID-ID_TOOLSETTINGS_HARDNESS1; RECT r=d->rcItem; COLORREF bg=(d->itemState&ODS_SELECTED)?RGB(70,78,95):(temaAtual==1?RGB(245,245,245):RGB(40,42,48)); HBRUSH b=CreateSolidBrush(bg);FillRect(d->hDC,&r,b);DeleteObject(b); HPEN p=CreatePen(PS_SOLID,1,(ix==durezaPincel)?RGB(70,170,245):RGB(110,115,125));HGDIOBJ op=SelectObject(d->hDC,p);HGDIOBJ ob=SelectObject(d->hDC,GetStockObject(HOLLOW_BRUSH));Rectangle(d->hDC,r.left,r.top,r.right,r.bottom);int cx=(r.left+r.right)/2,cy=r.top+11;int maxR=8; for(int ring=0;ring<4;++ring){int rr=maxR-ring*2; if(rr<2)continue; HBRUSH rb=CreateSolidBrush(ring<=ix?RGB(70+ring*28,75+ring*25,88+ring*25):RGB(185,190,200)); HGDIOBJ oldb=SelectObject(d->hDC,rb);Ellipse(d->hDC,cx-rr,cy-rr,cx+rr+1,cy+rr+1);SelectObject(d->hDC,oldb);DeleteObject(rb);} SetBkMode(d->hDC,TRANSPARENT); SetTextColor(d->hDC,temaAtual==1?RGB(30,30,30):RGB(235,235,240)); RECT tr=r; tr.top+=21; DrawTextW(d->hDC,ix==0?L"Macio":ix==1?L"Suave":ix==2?L"Firme":L"Duro",-1,&tr,DT_CENTER|DT_VCENTER|DT_SINGLELINE); SelectObject(d->hDC,ob);SelectObject(d->hDC,op);DeleteObject(p);return TRUE;} }
    if(msg==WM_COMMAND && LOWORD(wParam)>=ID_TOOLSETTINGS_HARDNESS1 && LOWORD(wParam)<=ID_TOOLSETTINGS_HARDNESS4){ durezaPincel=(int)LOWORD(wParam)-ID_TOOLSETTINGS_HARDNESS1; for(int hi=0;hi<4;++hi) InvalidateRect(GetDlgItem(hwnd,ID_TOOLSETTINGS_HARDNESS1+hi),NULL,TRUE); return 0; }
    if(msg==WM_COMMAND && LOWORD(wParam)==ID_TOOLSETTINGS_LASSO_MODE && HIWORD(wParam)==CBN_SELCHANGE){
        int m=(int)SendMessageW(GetDlgItem(hwnd,ID_TOOLSETTINGS_LASSO_MODE),CB_GETCURSEL,0,0);
        modoLaco=(m==1)?1:0;
        SalvarConfiguracoesUsuario();
        InvalidarCacheComposicaoCanvas(); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
        return 0;
    }
    if(msg==WM_COMMAND && LOWORD(wParam)==ID_TOOLSETTINGS_LASSO_COLOR){
        CHOOSECOLORW cc{}; static COLORREF custom[16]={0}; cc.lStructSize=sizeof(cc);cc.hwndOwner=hwnd;cc.lpCustColors=custom;cc.rgbResult=corSelecao;cc.Flags=CC_FULLOPEN|CC_RGBINIT|CC_ENABLEHOOK;cc.lpfnHook=KodaHookSeletorCor;
        if(ChooseColorW(&cc)){corSelecao=cc.rgbResult;SalvarConfiguracoesUsuario();InvalidarCacheComposicaoCanvas(); if(hwndCanvas)InvalidateRect(hwndCanvas,NULL,FALSE);}return 0;
    }
    if(msg==WM_COMMAND && (LOWORD(wParam)==ID_TOOLSETTINGS_CLOSE || LOWORD(wParam)==ID_TOOLSETTINGS_OK)){SalvarEFecharPreferenciasFerramenta(hwnd);return 0;}
    if(msg==WM_TIMER&&wParam==1){
        if(ferramentaPropriedadesAtual!=ID_TOOL_LACON){ POINT pt{}; GetCursorPos(&pt); HWND under=WindowFromPoint(pt);
            bool inside=(under==hwnd || (under && IsChild(hwnd,under)));
            if(!inside) { SalvarEFecharPreferenciasFerramenta(hwnd); return 0; }
        } return 0;
    }
    if(msg==WM_CLOSE){SalvarEFecharPreferenciasFerramenta(hwnd);return 0;}
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK SubclassBotaoFerramenta(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR refData) {
    if(msg==WM_CONTEXTMENU) return 0;
    if(msg==WM_LBUTTONDBLCLK && (int)refData==ID_TOOL_LAPIS){ AbrirGerenciadorPincel(GetParent(hwnd)); return 0; }
    if(msg==WM_RBUTTONUP){
        HWND parent=GetParent(hwnd); int id=(int)refData;
        if(id==ID_TOOL_LAPIS){ferramentaAtual='L';ferramentaSelecionadaID=ID_TOOL_LAPIS;} else if(id==ID_TOOL_BORRACHA){ferramentaAtual='E';ferramentaSelecionadaID=ID_TOOL_BORRACHA;} else if(id==ID_TOOL_BALDE){ferramentaAtual='B';ferramentaSelecionadaID=ID_TOOL_BALDE;} else if(id==ID_TOOL_LACON){ferramentaAtual='A';ferramentaSelecionadaID=ID_TOOL_LACON;}
        if(parent) {
            for(int tid : {ID_TOOL_LAPIS,ID_TOOL_BORRACHA,ID_TOOL_BALDE,ID_TOOL_LACON,ID_TOOL_RODA_CORES,ID_TOOL_CONTA_GOTAS,ID_BTN_RENDERIZAR}) { HWND b=GetDlgItem(parent,tid); if(b) InvalidateRect(b,NULL,TRUE); }
            AbrirPreferenciasFerramenta(parent,id);
            // Nas bandejas que fecham ao sair (borracha/balde), posiciona a mira
            // no centro apenas uma vez ao abrir. Lápis e laço não usam esse efeito.
            if((id==ID_TOOL_BORRACHA || id==ID_TOOL_BALDE) && hwndToolSettings && IsWindow(hwndToolSettings)) {
                RECT pr{}; if(GetWindowRect(hwndToolSettings,&pr)) SetCursorPos((pr.left+pr.right)/2,(pr.top+pr.bottom)/2);
            }
        } return 0;
    }
    return DefSubclassProc(hwnd,msg,wParam,lParam);
}

// Desenha ícones minimalistas com GDI, sem arquivos externos e com hitbox do botão intacta.
void DesenharIconeFerramenta(DRAWITEMSTRUCT* dis) {
    if (!dis || !dis->hDC) return;
    RECT r=dis->rcItem;
    bool pressed=(dis->itemState & ODS_SELECTED)!=0;
    bool active=((int)dis->CtlID==ferramentaSelecionadaID);
    COLORREF normal=temaAtual==1?RGB(245,245,245):temaAtual==2?RGB(255,230,241):RGB(38,40,46);
    COLORREF selecionada=temaAtual==2?RGB(249,218,231):temaAtual==1?RGB(228,232,238):RGB(45,49,57);
    HBRUSH bg=CreateSolidBrush((active||pressed)?selecionada:normal);
    FillRect(dis->hDC,&r,bg); DeleteObject(bg);
    HPEN border=CreatePen(PS_SOLID,1,(active||pressed)?(temaAtual==2?RGB(218,173,190):RGB(95,105,120)):RGB(100,105,115));
    HGDIOBJ oldPen=SelectObject(dis->hDC,border); HGDIOBJ oldBrush=SelectObject(dis->hDC,GetStockObject(HOLLOW_BRUSH));
    Rectangle(dis->hDC,r.left,r.top,r.right,r.bottom); SelectObject(dis->hDC,oldPen); DeleteObject(border);
    int cx=(r.left+r.right)/2, cy=(r.top+r.bottom)/2;
    COLORREF iconColor=(temaAtual==0||temaAtual==3)?RGB(230,232,238):RGB(45,52,64);
    HPEN iconPen=CreatePen(PS_SOLID,2,iconColor); oldPen=SelectObject(dis->hDC,iconPen);
    int id=(int)dis->CtlID;
    if(id==ID_TOOL_LAPIS) {
        POINT pts[4]={{cx-10,cy+8},{cx+7,cy-9},{cx+11,cy-5},{cx-6,cy+12}};
        Polygon(dis->hDC,pts,4); MoveToEx(dis->hDC,cx-10,cy+8,NULL); LineTo(dis->hDC,cx-6,cy+12);
        MoveToEx(dis->hDC,cx+4,cy-6,NULL); LineTo(dis->hDC,cx+8,cy-2);
    } else if(id==ID_TOOL_BORRACHA) {
        POINT pts[4]={{cx-10,cy+3},{cx-1,cy-8},{cx+10,cy+1},{cx+1,cy+10}}; Polygon(dis->hDC,pts,4); MoveToEx(dis->hDC,cx-4,cy+7,NULL); LineTo(dis->hDC,cx+6,cy-4);
    } else if(id==ID_TOOL_BALDE) {
        MoveToEx(dis->hDC,cx-8,cy-4,NULL); LineTo(dis->hDC,cx+4,cy-12); LineTo(dis->hDC,cx+11,cy-2); LineTo(dis->hDC,cx-1,cy+6); LineTo(dis->hDC,cx-8,cy-4);
        MoveToEx(dis->hDC,cx-4,cy+7,NULL); LineTo(dis->hDC,cx+7,cy+7); LineTo(dis->hDC,cx+4,cy+12); LineTo(dis->hDC,cx-1,cy+12); LineTo(dis->hDC,cx-4,cy+7);
    } else if(id==ID_TOOL_LACON) {
        Arc(dis->hDC,cx-11,cy-11,cx+11,cy+11, cx+7,cy-8,cx-7,cy-8); MoveToEx(dis->hDC,cx-7,cy-8,NULL); LineTo(dis->hDC,cx-4,cy-3); LineTo(dis->hDC,cx+1,cy-6);
    } else if(id==ID_TOOL_RODA_CORES) {
        HBRUSH colors[3]={CreateSolidBrush(RGB(210,70,70)),CreateSolidBrush(RGB(70,150,100)),CreateSolidBrush(RGB(70,115,210))};
        for(int i=0;i<3;++i){ HGDIOBJ op=SelectObject(dis->hDC,colors[i]); Ellipse(dis->hDC,cx-10+i*5,cy-8+i*2,cx+1+i*5,cy+3+i*2); SelectObject(dis->hDC,oldBrush); DeleteObject(colors[i]); }
    } else if(id==ID_TOOL_CONTA_GOTAS) {
        MoveToEx(dis->hDC,cx-7,cy+7,NULL); LineTo(dis->hDC,cx+5,cy-5); LineTo(dis->hDC,cx+9,cy-9); LineTo(dis->hDC,cx+12,cy-6); LineTo(dis->hDC,cx+8,cy-2); LineTo(dis->hDC,cx-4,cy+10); LineTo(dis->hDC,cx-7,cy+7); Ellipse(dis->hDC,cx-10,cy+7,cx-5,cy+12);
    } else if(id==ID_BTN_RENDERIZAR) {
        POINT tri[3]={{cx-5,cy-10},{cx+9,cy},{cx-5,cy+10}}; Polygon(dis->hDC,tri,3);
    }
    SelectObject(dis->hDC,oldPen); DeleteObject(iconPen); SelectObject(dis->hDC,oldBrush);
}

static HWND g_hwndToolsTooltip = NULL;
static std::wstring g_toolsTooltipText;
static const wchar_t* KodaNomeFerramentaPorId(int id) {
    switch(id) {
        case ID_TOOL_LAPIS: return L"Lápis / pincéis";
        case ID_TOOL_BORRACHA: return L"Borracha";
        case ID_TOOL_BALDE: return L"Balde de preenchimento";
        case ID_TOOL_LACON: return L"Laço de seleção";
        case ID_TOOL_RODA_CORES: return L"Escolher cor";
        case ID_TOOL_CONTA_GOTAS: return L"Conta-gotas";
        case ID_BTN_RENDERIZAR: return L"Renderizar animação";
    }
    return L"";
}
static void KodaCriarDicasFerramentas(HWND parent) {
    if(!parent || g_hwndToolsTooltip) return;
    g_hwndToolsTooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,NULL,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,parent,NULL,hInstGlobal,NULL);
    if(!g_hwndToolsTooltip) return;
    SendMessageW(g_hwndToolsTooltip,TTM_SETMAXTIPWIDTH,0,260);
    const int ids[]={ID_TOOL_LAPIS,ID_TOOL_BORRACHA,ID_TOOL_BALDE,ID_TOOL_LACON,ID_TOOL_RODA_CORES,ID_TOOL_CONTA_GOTAS,ID_BTN_RENDERIZAR};
    for(size_t i=0;i<sizeof(ids)/sizeof(ids[0]);++i) {
        HWND b=GetDlgItem(parent,ids[i]); if(!b) continue;
        TOOLINFOW ti{}; ti.cbSize=sizeof(ti); ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS; ti.hwnd=parent; ti.uId=(UINT_PTR)b;
        ti.lpszText=(LPWSTR)KodaNomeFerramentaPorId(ids[i]); SendMessageW(g_hwndToolsTooltip,TTM_ADDTOOLW,0,(LPARAM)&ti);
    }
}

LRESULT CALLBACK WndProcTools(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_MOUSEMOVE) ReativarTimerFocoAutomatico();
    if(msg==WM_NOTIFY && lParam) {
        NMHDR* hdr=(NMHDR*)lParam;
        if(hdr->code==TTN_GETDISPINFOW) {
            NMTTDISPINFOW* info=(NMTTDISPINFOW*)lParam;
            int id=(int)GetDlgCtrlID((HWND)hdr->idFrom);
            g_toolsTooltipText=KodaTraduzirTexto(KodaNomeFerramentaPorId(id));
            info->lpszText=(LPWSTR)g_toolsTooltipText.c_str(); return 0;
        }
    }
    if (msg==WM_PAINT) {
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); RECT rc{}; GetClientRect(hwnd,&rc);
        HBRUSH bg=CreateSolidBrush(CorTemaFundo()); FillRect(dc,&rc,bg); DeleteObject(bg);
        HPEN edge=CreatePen(PS_SOLID,1,RGB(72,76,92)); HGDIOBJ old=SelectObject(dc,edge);
        HGDIOBJ oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH)); Rectangle(dc,0,0,rc.right,rc.bottom);
        SelectObject(dc,oldBrush); SelectObject(dc,old); DeleteObject(edge); EndPaint(hwnd,&ps); return 0;
    }
    if (msg==WM_DRAWITEM) { DRAWITEMSTRUCT* dis=(DRAWITEMSTRUCT*)lParam; if(dis && (dis->CtlID==ID_TOOL_LAPIS || dis->CtlID==ID_TOOL_BORRACHA || dis->CtlID==ID_TOOL_BALDE || dis->CtlID==ID_TOOL_LACON || dis->CtlID==ID_TOOL_RODA_CORES || dis->CtlID==ID_TOOL_CONTA_GOTAS || dis->CtlID==ID_BTN_RENDERIZAR)){ DesenharIconeFerramenta(dis); return TRUE; } }
    if (msg==WM_CLOSE) { ShowWindow(hwnd,SW_HIDE); return 0; }
    switch (msg) {
        case WM_COMMAND:
            // Fallback para classes/temas de botão que enviem BN_DOUBLECLICKED
            // ao pai em vez de entregar WM_LBUTTONDBLCLK ao subclass.
            if (LOWORD(wParam)==ID_TOOL_LAPIS && HIWORD(wParam)==BN_DOUBLECLICKED) {
                AbrirGerenciadorPincel(hwnd);
                return 0;
            }
            switch (LOWORD(wParam)) {
                case ID_TOOL_LAPIS: ferramentaAtual = 'L'; ferramentaSelecionadaID=ID_TOOL_LAPIS; InvalidateRect(hwnd,NULL,TRUE); break;
                case ID_TOOL_BORRACHA: ferramentaAtual = 'E'; ferramentaSelecionadaID=ID_TOOL_BORRACHA; InvalidateRect(hwnd,NULL,TRUE); break;
                case ID_TOOL_BALDE: ferramentaAtual='B'; ferramentaSelecionadaID=ID_TOOL_BALDE; InvalidateRect(hwnd,NULL,TRUE); break;
                case ID_TOOL_LACON: ferramentaAtual='A'; ferramentaSelecionadaID=ID_TOOL_LACON; InvalidateRect(hwnd,NULL,TRUE); break;
                case ID_TOOL_CONTA_GOTAS: ferramentaAtual='I'; ferramentaSelecionadaID=ID_TOOL_CONTA_GOTAS; InvalidateRect(hwnd,NULL,TRUE); break;
                case ID_TOOL_RODA_CORES: {
                    CHOOSECOLORW cc{};
                    static COLORREF cores[16] = {0};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = cores;
                    cc.rgbResult = corAtual;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
                    cc.lpfnHook = KodaHookSeletorCor;
                    if (ChooseColorW(&cc)) corAtual = cc.rgbResult;
                    break;
                }
                case ID_BTN_RENDERIZAR:
                    RenderizarMP4(hwnd);
                    break;
            }
            for(int tid : {ID_TOOL_LAPIS,ID_TOOL_BORRACHA,ID_TOOL_BALDE,ID_TOOL_LACON,ID_TOOL_RODA_CORES,ID_TOOL_CONTA_GOTAS,ID_BTN_RENDERIZAR}) { HWND hb=GetDlgItem(hwnd,tid); if(hb) InvalidateRect(hb,NULL,TRUE); }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    hInstGlobal = hInstance;
    CarregarConfiguracoesUsuario();
    DetectarRecursosCPU();
    PersistirDiagnosticoCPUIni();
    DetectarMesaDigitalizadoraUmaVez(); // enumeração PnP/driver é feita uma única vez por inicialização
    AplicarLimiteNucleos(nucleosDesejados);
    AplicarLimiteRAM(limiteRAMMB);
    if (fontePreferencias) { DeleteObject(fontePreferencias); fontePreferencias=NULL; }
    LOGFONTW fonteInicial{}; fonteInicial.lfHeight=-((12*tamanhoTexto+50)/100); fonteInicial.lfWeight=FW_NORMAL;
    lstrcpyW(fonteInicial.lfFaceName,L"Segoe UI"); fontePreferencias=CreateFontIndirectW(&fonteInicial);

    // Carrega o ícone 1 incorporado pelo recursos.rc
    hIconeApp = (HICON)LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(1),
        IMAGE_ICON,
        32,
        32,
        LR_DEFAULTCOLOR
    );

    // Fallback caso o ícone personalizado não seja encontrado
    if (!hIconeApp) {
        hIconeApp = LoadIcon(NULL, IDI_APPLICATION);
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProcMain;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));

    // Ícone personalizado da janela
    wc.hIcon = hIconeApp;
    wc.hIconSm = hIconeApp;

    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"KodanimaMainClass";

    if (!RegisterClassExW(&wc) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        MessageBoxW(NULL,
            L"Não foi possível registrar a janela principal.",
            L"Kodanima", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowW(
        L"KodanimaMainClass",
        L"Kodanima 2D",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        520, 390,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL,
            L"Não foi possível criar a janela principal.",
            L"Kodanima", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (fontePreferencias) EnumChildWindows(hwnd, AplicarFonteControle, (LPARAM)fontePreferencias);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    auto cursorSobreJanela = [](HWND target)->bool {
        if(!target || !IsWindow(target) || !IsWindowVisible(target)) return false;
        POINT p{};
        if(!GetCursorPos(&p)) return false;
        HWND under=WindowFromPoint(p);
        if (pressureApi != 0) {
            // Preserva o comportamento anterior do Windows Pointer/Ink.
            for(HWND cur=under; cur; cur=GetParent(cur)) if(cur==target) return true;
        } else {
            // No modo Wintab, nao confunde dialogos popup com a janela dona.
            for(HWND cur=under; cur; ) {
                if(cur==target) return true;
                if(!(GetWindowLongPtrW(cur,GWL_STYLE)&WS_CHILD)) break;
                cur=GetParent(cur);
            }
        }
        return false; // no captura atalhos quando outra janela real está sobre o canvas
    };
    // Garante que a mira do modo Wintab continue visivel sobre todos os controles
    // do proprio programa (inclusive botoes filhos da timeline/ferramentas).
    g_wintabCursorHook=SetWindowsHookExW(WH_CALLWNDPROCRET,WintabCursorCallWndRetProc,NULL,GetCurrentThreadId());
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if(msg.message==WM_DROPFILES) { KodaProcessarDrop(msg.hwnd,(HDROP)msg.wParam); continue; }
        HWND focoAtual=GetFocus();
        wchar_t classeFoco[64]={0};
        if (focoAtual && IsWindow(focoAtual)) GetClassNameW(focoAtual,classeFoco,63);
        const bool focoEmCampoTexto=(lstrcmpiW(classeFoco,L"Edit")==0 ||
            lstrcmpiW(classeFoco,L"ComboBox")==0 || wcsstr(classeFoco,L"RichEdit")!=NULL);
        HWND foreground=GetForegroundWindow();
        const bool kodanimaForeground=(foreground==hwndCanvas || foreground==hwndTimeline || foreground==hwndTools || foreground==hwndMain || foreground==hwndSettings || foreground==hwndShortcutPanel);
        if(msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 && msg.wParam=='V' &&
           (GetAsyncKeyState(VK_CONTROL)&0x8000) && (GetAsyncKeyState(VK_SHIFT)&0x8000) && !focoEmCampoTexto && kodanimaForeground) {
            KodaColarImagemClipboard(hwndCanvas?hwndCanvas:(hwndTimeline?hwndTimeline:hwndTools));
            continue;
        }
        // Atalhos universais da aplicação: vírgula = quadro anterior; ponto = próximo.
        // Não intercepta quando um campo de edição está focado, especialmente campos numéricos
        // onde vírgulas/pontos fazem parte do valor digitado.
        if (msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 &&
            (msg.wParam==g_kodaShortcuts[0].vk || msg.wParam==g_kodaShortcuts[1].vk) && !focoEmCampoTexto &&
            hwndTimeline && IsWindow(hwndTimeline) && !listaFrames.empty()) {
            int passo=(msg.wParam==g_kodaShortcuts[0].vk)?-1:1;
            for(int i=frameAtual+passo;i>=0 && i<(int)listaFrames.size();i+=passo) {
                if(QuadroExisteNaCamada(listaFrames[i],selectedTimelineLayer)) { IrParaQuadro(i); break; }
            }
            if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
            if(hwndTimeline) InvalidateRect(hwndTimeline,NULL,FALSE);
            continue;
        }
        // Atalhos configuráveis adicionais para os controles e ferramentas principais.
        if(msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 && !focoEmCampoTexto){
            for(int si=8;si<g_kodaShortcutCount;++si){ if(g_kodaShortcuts[si].fixed || msg.wParam!=g_kodaShortcuts[si].vk) continue;
                switch(si){
                    case 8: if(hwndTimeline)SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_BTN_NOVO_QUADRO,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_BTN_NOVO_QUADRO)); break;
                    case 9: if(hwndTimeline&&GetDlgItem(hwndTimeline,ID_BTN_ONION))SendMessageW(GetDlgItem(hwndTimeline,ID_BTN_ONION),BM_CLICK,0,0); break;
                    case 10: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_BTN_RENDERIZAR,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_BTN_RENDERIZAR)); break;
                    case 11: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_TOOL_LAPIS,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_TOOL_LAPIS)); break;
                    case 12: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_TOOL_BORRACHA,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_TOOL_BORRACHA)); break;
                    case 13: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_TOOL_BALDE,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_TOOL_BALDE)); break;
                    case 14: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_TOOL_RODA_CORES,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_TOOL_RODA_CORES)); break;
                    case 15: if(hwndTools)SendMessageW(hwndTools,WM_COMMAND,MAKEWPARAM(ID_TOOL_CONTA_GOTAS,BN_CLICKED),(LPARAM)GetDlgItem(hwndTools,ID_TOOL_CONTA_GOTAS)); break;
                    case 16: if(hwndTimeline)SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_LAYER_ADD,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_LAYER_ADD)); break;
                    case 17: if(hwndTimeline)SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_LAYER_DELETE,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_LAYER_DELETE)); break;
                    case 18: if(hwndTimeline)SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_LAYER_UP,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_LAYER_UP)); break;
                    case 19: if(hwndTimeline)SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_LAYER_DOWN,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_LAYER_DOWN)); break;
                    case 20: KodaMostrarDialogoImportar(hwndTimeline?hwndTimeline:(hwndCanvas?hwndCanvas:hwndMain)); break;
                    case 22: // Num +: aumenta em 0,1 s a duração do quadro ativo na timeline
                    case 23: { // Num -: diminui em 0,1 s, sem permitir duração zero
                        if(hwndTimeline && cursorSobreJanela(hwndTimeline) && !listaFrames.empty()) {
                            int fi=selectedTimelineFrame;
                            if(fi<0 || fi>=(int)listaFrames.size()) fi=frameAtual;
                            int row=selectedTimelineLayer;
                            if(fi>=0 && fi<(int)listaFrames.size() && row>=0 && row<(int)timelineLayers.size() && QuadroExisteNaCamada(listaFrames[fi],row)) {
                                RegistrarHistoricoTimeline();
                                GarantirBitmapsCamadas(listaFrames[fi]);
                                double oldDur=DuracaoCamadaQuadro(listaFrames[fi],row);
                                double delta=(si==22)?0.1:-0.1;
                                double newDur=std::max(0.02,std::min(3600.0,oldDur+delta));
                                if(row<(int)listaFrames[fi].layerDurations.size()) listaFrames[fi].layerDurations[row]=newDur;
                                if(row==0) listaFrames[fi].segundos=newDur;
                                frameAtual=fi; selectedTimelineFrame=fi; IrParaQuadro(fi);
                                AtualizarCampoDuracao(hwndTimeline); SalvarProjetoCompleto();
                                InvalidateRect(hwndTimeline,NULL,FALSE); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
                            }
                        }
                        break;
                    }
                    case 24: // Num 4: desloca o quadro selecionado para a esquerda
                    case 25: { // Num 6: desloca o quadro selecionado para a direita
                        if(hwndTimeline && cursorSobreJanela(hwndTimeline) && listaFrames.size()>1) {
                            int source=selectedTimelineFrame;
                            if(source<0 || source>=(int)listaFrames.size()) source=frameAtual;
                            int target=source+((si==24)?-1:1);
                            if(source>=0 && source<(int)listaFrames.size() && target>=0 && target<(int)listaFrames.size()) {
                                RegistrarHistoricoTimeline();
                                if(source<target) std::rotate(listaFrames.begin()+source,listaFrames.begin()+source+1,listaFrames.begin()+target+1);
                                else std::rotate(listaFrames.begin()+target,listaFrames.begin()+source,listaFrames.begin()+source+1);
                                for(int fi=0;fi<(int)listaFrames.size();++fi) listaFrames[fi].id=fi+1;
                                frameAtual=target; selectedTimelineFrame=target; IrParaQuadro(target);
                                SalvarProjetoCompleto(); InvalidateRect(hwndTimeline,NULL,FALSE); if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
                            }
                        }
                        break;
                    }
                }
                continue;
            }
        }
        // Registra Espaço globalmente; o foco pode continuar na janela da timeline
        // enquanto a ponta Wintab sintetiza mouse down/move sobre o canvas.
        if(msg.message==WM_KEYDOWN && msg.wParam==VK_SPACE) g_spacePanModifierDown=true;
        if(msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 && (msg.wParam=='P' || msg.wParam=='p')) {
            g_atalhoCentralizarPPressionadoEm=GetTickCount();
        } else if(msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 &&
                  (msg.wParam=='0' || msg.wParam==VK_NUMPAD0) &&
                  g_atalhoCentralizarPPressionadoEm &&
                  (DWORD)(GetTickCount()-g_atalhoCentralizarPPressionadoEm)<=1500 &&
                  (cursorSobreJanela(hwndCanvas) || cursorSobreJanela(hwndTimeline) || cursorSobreJanela(hwndTools))) {
            panX=0; panY=0; g_atalhoCentralizarPPressionadoEm=0;
            if(hwndCanvas && IsWindow(hwndCanvas)){InvalidateRect(hwndCanvas,NULL,FALSE);UpdateWindow(hwndCanvas);}
            continue;
        } else if(msg.message==WM_KEYDOWN && (msg.lParam&(1L<<30))==0 &&
                  msg.wParam!='0' && msg.wParam!=VK_NUMPAD0) {
            g_atalhoCentralizarPPressionadoEm=0;
        }
        if(msg.message==WM_KEYUP && msg.wParam==VK_SPACE) {
            g_spacePanModifierDown=false;
            if(g_wintabSpacePanHoverActive) {
                g_wintabSpacePanHoverActive=false;
                navegandoCanvas=false;
                if(hwndCanvas && GetCapture()==hwndCanvas) ReleaseCapture();
                if(hwndCanvas) InvalidateRect(hwndCanvas,NULL,FALSE);
            }
        }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam == g_kodaShortcuts[5].vk && hwndCanvas && IsWindow(hwndCanvas)) { AlternarCanvasMaximizado(); continue; }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam == g_kodaShortcuts[3].vk) { AbrirConfiguracoesUsuario(hwndCanvas ? hwndCanvas : hwndMain); continue; }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam == VK_F11) {
            // Usa a mesma rotina de cancelamento do canvas para também restaurar
            // o backup do objeto em transformação e liberar a máscara/cache do laço.
            if(hwndCanvas && IsWindow(hwndCanvas)) SendMessageW(hwndCanvas, WM_KEYDOWN, VK_F11, 0);
            else { selecaoAtiva=false; criandoSelecao=false; lacoAtivo=false; operacaoSelecao=0; }
            continue;
        }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam == g_kodaShortcuts[4].vk && hwndTimeline && IsWindow(hwndTimeline)) { SendMessageW(hwndTimeline, WM_COMMAND, MAKEWPARAM(ID_BTN_INSERIR_MEIO, BN_CLICKED), (LPARAM)GetDlgItem(hwndTimeline, ID_BTN_INSERIR_MEIO)); continue; }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam == g_kodaShortcuts[6].vk &&
            !focoEmCampoTexto && (cursorSobreJanela(hwndCanvas) || cursorSobreJanela(hwndTimeline))) {
            AplicarDeletePelaPosicaoDoMouse(); continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            // Esc primeiro cancela laço/transformação/colagem; só fecha as janelas
            // quando não há uma operação de seleção para cancelar.
            if(hwndCanvas && IsWindow(hwndCanvas) &&
               (selecaoAtiva || criandoSelecao || lacoAtivo || transformacaoSelecaoAtiva || colandoSelecao))
                SendMessageW(hwndCanvas, WM_KEYDOWN, VK_ESCAPE, 0);
            else
                FecharTodasJanelas();
            continue;
        }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && msg.wParam==g_kodaShortcuts[2].vk && !focoEmCampoTexto &&
            !((GetKeyState(VK_CONTROL)&0x8000)!=0) && !((GetAsyncKeyState(VK_CONTROL)&0x8000)!=0) && hwndTimeline && IsWindow(hwndTimeline)) {
            POINT cursor{}; GetCursorPos(&cursor); ScreenToClient(hwndTimeline,&cursor);
            if(cursor.x>=TIMELINE_SPLIT_X && cursor.y>=TIMELINE_RULER_TOP && cursor.y<TIMELINE_RULER_TOP+alturaReguaTimeline+((int)timelineLayers.size()+2)*alturaLinhaCamada){
                SendMessageW(hwndTimeline,WM_COMMAND,MAKEWPARAM(ID_BTN_PLAY_PREVIEW,BN_CLICKED),(LPARAM)GetDlgItem(hwndTimeline,ID_BTN_PLAY_PREVIEW));
                continue;
            }
        }
        if (msg.message == WM_KEYDOWN && (msg.lParam & (1L<<30)) == 0 && !focoEmCampoTexto &&
            (((GetKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0))) {
            if (msg.wParam == VK_RETURN) {
                if (hwndTimeline && IsWindow(hwndTimeline))
                    SendMessageW(hwndTimeline, WM_COMMAND, MAKEWPARAM(ID_BTN_PLAY_PREVIEW, BN_CLICKED), (LPARAM)GetDlgItem(hwndTimeline, ID_BTN_PLAY_PREVIEW));
                continue;
            }
            if (msg.wParam == 'Z') {
                // Drena primeiro a fila Wintab: o WT_PACKET de soltura pode estar
                // pendente atrás do WM_KEYDOWN, causando o antigo Ctrl+Z que falhava.
                if (pressureApi==0 && g_wintabContext && hwndCanvas)
                    WintabProcessarFilaPendente(hwndCanvas);
                if (desenhando || canetaPointerEmContato || (pressureApi==0 && g_wintabContactActive)) continue;
                if(cursorSobreJanela(hwndTimeline) && KodaRestaurarUltimaExclusaoMedia()) continue;
                if(cursorSobreJanela(hwndTimeline)) DesfazerTimeline(false);
                else if(cursorSobreJanela(hwndCanvas) || (pressureApi==0 && cursorSobreJanela(hwndTools))) ExecutarUndo();
                else continue; // permite o Ctrl+Z nativo em campos de texto/janelas de diálogo
                continue;
            }
            if (msg.wParam == 'Y') {
                if (pressureApi==0 && g_wintabContext && hwndCanvas)
                    WintabProcessarFilaPendente(hwndCanvas);
                if (desenhando || canetaPointerEmContato || (pressureApi==0 && g_wintabContactActive)) continue;
                if(cursorSobreJanela(hwndTimeline)) DesfazerTimeline(true);
                else if(cursorSobreJanela(hwndCanvas) || (pressureApi==0 && cursorSobreJanela(hwndTools))) ExecutarRedo();
                else continue; // permite o Ctrl+Y nativo fora do canvas/timeline
                continue;
            }
            if (msg.wParam == 'S') { SalvarProjetoCompleto(); continue; }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_wintabCursorHook) {
        UnhookWindowsHookEx(g_wintabCursorHook);
        g_wintabCursorHook=NULL;
    }
    return (int)msg.wParam;
}


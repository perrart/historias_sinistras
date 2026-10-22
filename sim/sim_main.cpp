// Simulador de PC: executa o jogo real (mesmo código do firmware) com um roteiro
// de teclas, salva capturas de tela e verifica regras de conteúdo e navegação.
//
//   make -C sim && ./sim/build/hs_sim sim/out
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

#include "../src/Config.h"
#include "../src/Fonts.h"
#include "../src/GameEngine.h"
#include "../src/MysteryDatabase.h"
#include "../src/StoryRenderer.h"
#include "Sim.h"

static std::string g_out;
static int g_fail = 0;

#define CHECK(cond, ...)                         \
  do {                                           \
    if (!(cond)) {                               \
      ++g_fail;                                  \
      printf("  FALHA: " __VA_ARGS__);           \
      printf("  (%s:%d)\n", __FILE__, __LINE__); \
    }                                            \
  } while (0)

static void run(uint32_t ms) {
  for (uint32_t t = 0; t < ms; t += 16) {
    sim::advance(16);
    Game.tick();
  }
}

static void press(uint8_t key) {
  KeySet k;
  k.set(key);
  sim::setHeld(k);
  run(48);
  sim::setHeld(KeySet());
  run(64);
}

static void pressChar(char c) {
  if (c >= 'a' && c <= 'z') press((uint8_t)(K_A + (c - 'a')));
  else if (c >= '0' && c <= '9') press((uint8_t)(K_0 + (c - '0')));
}

static void shot(const char* name) {
  run(32);
  std::string p = g_out + "/" + name + ".ppm";
  sim::savePpm(p.c_str());
  printf("  captura: %s\n", name);
}

static const char* screenName(ScreenId s) {
  static const char* n[] = {"SPLASH", "MENU",     "INTRO",    "HUB",   "CLUES",    "TRUTH",
                            "BROWSER", "RESOLVED", "HOWTO", "SETTINGS", "RESET", "CREDITS"};
  return s < SCR_COUNT ? n[s] : "?";
}

static void expectScreen(ScreenId s) {
  CHECK(Game.current() == s, "tela esperada %s, atual %s\n", screenName(s), screenName(Game.current()));
}

// ---------------------------------------------------------------------------
// Auditoria de conteúdo: cobertura de glifos e quebra de linha em todas as fontes
// ---------------------------------------------------------------------------
static bool hasGlyph(FontId f, uint32_t cp) {
  auto* vf = (const lgfx::VLWfont*)Fonts::get(f);
  uint16_t idx;
  return cp == ' ' || vf->getUnicodeIndex((uint16_t)cp, &idx);
}

static void auditText(const char* label, int id, const char* s, FontId f) {
  const char* p = s;
  const char* end = s + strlen(s);
  while (p < end) {
    const uint32_t cp = Fonts::nextCodepoint(p, end);
    if (cp == '\n') continue;
    if (!hasGlyph(f, cp)) {
      CHECK(false, "caso %d (%s): glifo U+%04X ausente na fonte %d\n", id, label, cp, f);
      return;
    }
  }
}

static void auditLayout(const char* label, int id, const char* s, FontId f, int width, int& maxLines) {
  static Document d;
  d.clear();
  d.addText(s, f, 0xFFFF, 0);
  d.layout(width);
  CHECK(!d.truncated(), "caso %d (%s): texto truncado\n", id, label);
  for (int i = 0; i < d.lineCount(); ++i) {
    const auto& ln = d.line(i);
    const int w = Fonts::textWidth(f, s + ln.start, ln.len);
    CHECK(w <= width, "caso %d (%s): linha %d com %dpx > %dpx\n", id, label, i, w, width);
  }
  if (d.lineCount() > maxLines) maxLines = d.lineCount();
}

static void auditContent() {
  printf("[auditoria de conteúdo]\n");
  const int viewW = cfg::SCREEN_W - 2 * cfg::MARGIN_X - 4;
  int maxSol = 0, maxScen = 0;
  CHECK(MysteryDB::count() >= 60, "esperados >= 60 casos, há %d\n", MysteryDB::count());
  for (int i = 0; i < MysteryDB::count(); ++i) {
    const Mystery& m = MysteryDB::at(i);
    CHECK(m.title && *m.title && m.scenario && *m.scenario && m.solution && *m.solution, "caso %d incompleto\n", m.id);
    for (int k = 0; k < 4; ++k) CHECK(m.clues[k] && strlen(m.clues[k]) > 20, "caso %d: dica %d vazia\n", m.id, k + 1);
    for (int j = i + 1; j < MysteryDB::count(); ++j)
      CHECK(MysteryDB::at(j).id != m.id, "ID duplicado %d\n", m.id);
    auditText("título", m.id, m.title, F_TITLE);
    const FontId bodies[2] = {F_BODY, F_BODY_L};
    for (FontId f : bodies) {
      auditText("cenário", m.id, m.scenario, f);
      auditText("verdade", m.id, m.solution, f);
      for (int k = 0; k < 4; ++k) auditText("dica", m.id, m.clues[k], f);
      auditLayout("cenário", m.id, m.scenario, f, viewW, maxScen);
      auditLayout("verdade", m.id, m.solution, f, viewW, maxSol);
      for (int k = 0; k < 4; ++k) {
        int dummy = 0;
        auditLayout("dica", m.id, m.clues[k], f, viewW, dummy);
      }
    }
    CHECK(m.category < CAT_COUNT && m.difficulty < DIFF_COUNT, "caso %d: categoria/dificuldade inválida\n", m.id);
  }
  printf("  %d casos auditados; maior cenário: %d linhas; maior verdade: %d linhas\n", MysteryDB::count(), maxScen,
         maxSol);
}

int main(int argc, char** argv) {
  g_out = argc > 1 ? argv[1] : "out";
  mkdir(g_out.c_str(), 0755);  // garante a pasta das capturas e da NVS simulada
  std::string store = g_out + "/nvs.bin";
  remove(store.c_str());
  sim::setStorePath(store.c_str());

  Game.begin();
  auditContent();

  printf("[roteiro de navegação]\n");
  run(250);
  shot("01_abertura_inicio");
  run(3800);
  shot("02_abertura");
  press(K_ENTER);
  run(400);
  expectScreen(SCR_MENU);
  shot("03_menu");
  press(K_DOWN);
  press(K_DOWN);
  run(200);
  shot("04_menu_navegacao");
  press(K_UP);
  press(K_UP);

  // JOGAR → caso 001
  pressChar('1');
  expectScreen(SCR_INTRO);
  CHECK(Game.caseId() == 1, "JOGAR deveria abrir o caso 001 (abriu %d)\n", Game.caseId());
  run(700);
  shot("05_caso_digitando");
  run(9000);
  shot("06_caso_apresentacao");
  press(K_ENTER);
  run(300);
  expectScreen(SCR_HUB);
  shot("07_investigacao");

  // dica 1
  pressChar('1');
  run(250);
  expectScreen(SCR_CLUES);
  shot("08_dica_carimbo");
  run(1300);
  run(5000);
  shot("09_dica_01");
  CHECK(Game.progress.clues(1) == 1, "dicas reveladas deveriam ser 1\n");
  // dicas 2-4 direto da tela de dicas
  pressChar('1');
  run(1300 + 6000);
  pressChar('1');
  run(1300 + 6000);
  pressChar('1');
  run(1300 + 6000);
  shot("10_dicas_04");
  CHECK(Game.progress.clues(1) == 4, "dicas reveladas deveriam ser 4 (são %d)\n", Game.progress.clues(1));
  pressChar('1');  // não deve passar de 4
  run(200);
  CHECK(Game.progress.clues(1) == 4, "não deveria haver 5ª dica\n");
  shot("11_sem_mais_dicas");
  press(K_UP);
  press(K_UP);
  press(K_UP);
  run(400);
  shot("12_dicas_rolagem");
  press(K_BACK);
  run(300);
  expectScreen(SCR_HUB);
  shot("13_investigacao_4_dicas");

  // reler o caso
  pressChar('3');
  run(400);
  expectScreen(SCR_INTRO);
  shot("14_reler_caso");
  press(K_BACK);
  run(300);
  expectScreen(SCR_HUB);

  // verdade: abre direto (sem tela de confirmação) e NÃO marca o caso como resolvido
  pressChar('2');
  run(300);
  expectScreen(SCR_TRUTH);
  shot("15_verdade_abertura");
  run(1400);
  run(16000);
  shot("16_verdade");
  CHECK(!Game.progress.resolved(1), "revelar a verdade não deveria resolver o caso sozinho\n");
  for (int i = 0; i < 6; ++i) press(K_RIGHT);
  run(600);
  shot("17_verdade_fim_sem_carimbo");
  CHECK(!Game.progress.resolved(1), "rolar até o fim não deveria resolver o caso\n");
  press(K_BACK);
  run(300);
  expectScreen(SCR_HUB);

  // marcar como resolvido pelo menu do caso (item 5)
  press(K_DOWN);
  press(K_DOWN);
  press(K_DOWN);
  run(100);
  shot("18_menu_marcar_resolvido");
  press(K_ENTER);
  run(250);
  CHECK(Game.progress.resolved(1), "o item 5 deveria marcar o caso 001 como resolvido\n");
  shot("19_marcado_carimbo");
  run(2600);
  shot("20_investigacao_resolvido");
  press(K_ENTER);  // desmarca
  run(300);
  CHECK(!Game.progress.resolved(1), "o item 5 deveria desmarcar o caso\n");
  shot("21_desmarcado");

  // atalho S na tela da verdade
  pressChar('2');
  run(300);
  expectScreen(SCR_TRUTH);
  run(1400);
  run(16000);
  pressChar('s');
  run(500);
  CHECK(Game.progress.resolved(1), "S na verdade deveria marcar o caso como resolvido\n");
  shot("22_verdade_marcada");

  // próximo caso
  pressChar('n');
  run(300);
  expectScreen(SCR_INTRO);
  CHECK(Game.caseId() == 2, "N deveria abrir o caso 002 (abriu %d)\n", Game.caseId());
  run(9000);
  shot("23_caso_002");
  pressChar('m');
  run(300);
  expectScreen(SCR_MENU);
  shot("24_menu_com_progresso");

  // arquivo de casos
  pressChar('2');
  run(300);
  expectScreen(SCR_BROWSER);
  shot("25_arquivo");
  for (int i = 0; i < 8; ++i) press(K_DOWN);
  shot("26_arquivo_rolagem");
  pressChar('3');
  pressChar('2');
  run(100);
  shot("27_arquivo_ir_para");
  press(K_ENTER);
  run(300);
  expectScreen(SCR_INTRO);
  CHECK(Game.caseId() == 32, "deveria abrir o caso 032 (abriu %d)\n", Game.caseId());
  run(9000);
  shot("28_caso_032");
  press(K_BACK);
  run(300);
  expectScreen(SCR_BROWSER);
  press(K_BACK);
  run(300);

  // resolvidos
  pressChar('3');
  run(300);
  expectScreen(SCR_RESOLVED);
  shot("29_resolvidos");
  press(K_BACK);
  run(300);

  // como jogar
  pressChar('4');
  run(300);
  expectScreen(SCR_HOWTO);
  shot("30_como_jogar");
  press(K_RIGHT);
  press(K_RIGHT);
  run(400);
  shot("31_como_jogar_pag3");
  press(K_BACK);
  run(300);

  // créditos
  pressChar('6');
  run(300);
  expectScreen(SCR_CREDITS);
  shot("32_creditos");
  press(K_BACK);
  run(300);
  expectScreen(SCR_MENU);

  // configurações: texto grande + instantâneo
  pressChar('5');
  run(300);
  expectScreen(SCR_SETTINGS);
  shot("33_configuracoes");
  press(K_DOWN);
  press(K_DOWN);
  press(K_RIGHT);
  press(K_RIGHT);  // velocidade → INSTANTÂNEA
  press(K_DOWN);
  press(K_RIGHT);  // tamanho → GRANDE
  run(100);
  shot("34_configuracoes_alteradas");
  CHECK(Game.settings.data().textSpeed == SPEED_INSTANT, "velocidade deveria ser INSTANTÂNEA\n");
  CHECK(Game.settings.data().textSize == SIZE_LARGE, "tamanho deveria ser GRANDE\n");
  press(K_BACK);
  run(300);
  pressChar('2');
  run(300);
  press(K_ENTER);  // abre o 032 (seleção mantida)
  run(500);
  shot("35_caso_texto_grande");
  press(K_ENTER);
  run(300);
  pressChar('2');
  run(400);
  expectScreen(SCR_TRUTH);
  press(K_ENTER);  // pula a abertura
  run(2000);
  shot("36_verdade_texto_grande");
  CHECK(!Game.progress.resolved(32), "032 não deveria estar resolvido antes de marcar\n");
  pressChar('s');
  run(300);
  CHECK(Game.progress.resolved(32), "032 deveria estar resolvido após S\n");
  pressChar('m');
  run(300);

  // persistência: "reinicia" o aparelho
  printf("[persistência]\n");
  const int writesBefore = sim::storageWrites;
  Game.progress.begin();
  Game.settings.begin();
  CHECK(Game.progress.resolved(1) && Game.progress.resolved(32), "progresso não sobreviveu ao reinício\n");
  CHECK(Game.progress.clues(1) == 4, "dicas do caso 001 não persistiram\n");
  CHECK(Game.settings.data().textSize == SIZE_LARGE, "configurações não persistiram\n");
  CHECK(Game.resolvedCount() == 2, "esperados 2 resolvidos, há %d\n", Game.resolvedCount());
  run(2000);
  CHECK(sim::storageWrites == writesBefore, "gravações desnecessárias na flash em repouso\n");
  printf("  gravações na NVS durante o roteiro: %d\n", sim::storageWrites);

  // zerar progresso
  pressChar('5');
  run(300);
  for (int i = 0; i < 5; ++i) press(K_DOWN);
  press(K_ENTER);
  run(300);
  expectScreen(SCR_RESET);
  shot("37_zerar_progresso");
  press(K_ENTER);  // cedo demais: deve ser ignorado
  CHECK(Game.resolvedCount() == 2, "confirmação imediata não deveria apagar\n");
  run(800);
  press(K_ENTER);
  run(300);
  CHECK(Game.resolvedCount() == 0, "progresso deveria ter sido apagado\n");
  expectScreen(SCR_SETTINGS);
  shot("38_progresso_apagado");

  printf("[resumo] quadros desenhados: %d, eventos de áudio: %d\n", sim::presents(), sim::audioEvents());
  if (g_fail) {
    printf("RESULTADO: %d FALHA(S)\n", g_fail);
    return 1;
  }
  printf("RESULTADO: TUDO OK\n");
  return 0;
}

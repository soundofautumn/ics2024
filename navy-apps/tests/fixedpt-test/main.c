#include <stdio.h>
#include <fixedptc.h>

static int passed, failed;

#define CHECK(cond, msg) do { \
  if (cond) { passed++; printf("  PASS: %s\n", msg); } \
  else { failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

int main() {
  printf("=== fixedptc test ===\n\n");

  /* fixedpt_muli: fixedpt * int */
  printf("--- fixedpt_muli ---\n");
  CHECK(fixedpt_muli(FIXEDPT_ONE * 3 / 2, 3) == FIXEDPT_ONE * 9 / 2, "1.5 * 3 == 4.5");
  CHECK(fixedpt_muli(FIXEDPT_ONE * 3, -2) == FIXEDPT_ONE * -6, "3 * -2 == -6");
  CHECK(fixedpt_muli(0, 42) == 0, "0 * 42 == 0");
  CHECK(fixedpt_muli(FIXEDPT_ONE * 5, 0) == 0, "5 * 0 == 0");

  /* fixedpt_divi: fixedpt / int */
  printf("--- fixedpt_divi ---\n");
  CHECK(fixedpt_divi(FIXEDPT_ONE * 9 / 2, 3) == FIXEDPT_ONE * 3 / 2, "4.5 / 3 == 1.5");
  CHECK(fixedpt_divi(FIXEDPT_ONE * -6, 2) == FIXEDPT_ONE * -3, "-6 / 2 == -3");
  CHECK(fixedpt_divi(0, 1) == 0, "0 / 1 == 0");

  /* fixedpt_mul: fixedpt * fixedpt */
  printf("--- fixedpt_mul ---\n");
  CHECK(fixedpt_mul(FIXEDPT_ONE * 3 / 2, FIXEDPT_ONE * 5 / 2) == FIXEDPT_ONE * 15 / 4, "1.5 * 2.5 == 3.75");
  CHECK(fixedpt_mul(FIXEDPT_ONE * 2, FIXEDPT_ONE * 3) == FIXEDPT_ONE * 6, "2 * 3 == 6");
  CHECK(fixedpt_mul(FIXEDPT_ONE * -2, FIXEDPT_ONE * 3) == FIXEDPT_ONE * -6, "-2 * 3 == -6");
  CHECK(fixedpt_mul(FIXEDPT_ONE * -2, FIXEDPT_ONE * -3) == FIXEDPT_ONE * 6, "-2 * -3 == 6");
  CHECK(fixedpt_mul(0, FIXEDPT_ONE * 5) == 0, "0 * 5 == 0");
  CHECK(fixedpt_mul(FIXEDPT_ONE, FIXEDPT_ONE) == FIXEDPT_ONE, "1 * 1 == 1");

  /* fixedpt_div: fixedpt / fixedpt */
  printf("--- fixedpt_div ---\n");
  CHECK(fixedpt_div(FIXEDPT_ONE * 15 / 4, FIXEDPT_ONE * 3 / 2) == FIXEDPT_ONE * 5 / 2, "3.75 / 1.5 == 2.5");
  CHECK(fixedpt_div(FIXEDPT_ONE * 6, FIXEDPT_ONE * 3) == FIXEDPT_ONE * 2, "6 / 3 == 2");
  CHECK(fixedpt_div(FIXEDPT_ONE * -6, FIXEDPT_ONE * 3) == FIXEDPT_ONE * -2, "-6 / 3 == -2");
  CHECK(fixedpt_div(FIXEDPT_ONE * -6, FIXEDPT_ONE * -3) == FIXEDPT_ONE * 2, "-6 / -3 == 2");
  CHECK(fixedpt_div(0, FIXEDPT_ONE * 5) == 0, "0 / 5 == 0");
  CHECK(fixedpt_div(FIXEDPT_ONE * 3, FIXEDPT_ONE) == FIXEDPT_ONE * 3, "3 / 1 == 3");

  /* fixedpt_abs */
  printf("--- fixedpt_abs ---\n");
  CHECK(fixedpt_abs(FIXEDPT_ONE * 3) == FIXEDPT_ONE * 3, "abs(3) == 3");
  CHECK(fixedpt_abs(FIXEDPT_ONE * -3) == FIXEDPT_ONE * 3, "abs(-3) == 3");
  CHECK(fixedpt_abs(0) == 0, "abs(0) == 0");
  CHECK(fixedpt_abs(FIXEDPT_ONE * -1) == FIXEDPT_ONE, "abs(-1) == 1");

  /* fixedpt_floor */
  printf("--- fixedpt_floor ---\n");
  CHECK(fixedpt_floor(FIXEDPT_ONE * 3 / 2) == FIXEDPT_ONE, "floor(1.5) == 1");
  CHECK(fixedpt_floor(FIXEDPT_ONE * 2) == FIXEDPT_ONE * 2, "floor(2.0) == 2");
  CHECK(fixedpt_floor(FIXEDPT_ONE * -3 / 2) == FIXEDPT_ONE * -2, "floor(-1.5) == -2");
  CHECK(fixedpt_floor(FIXEDPT_ONE * -2) == FIXEDPT_ONE * -2, "floor(-2.0) == -2");
  CHECK(fixedpt_floor(0) == 0, "floor(0) == 0");
  CHECK(fixedpt_floor(FIXEDPT_ONE * 7 / 4) == FIXEDPT_ONE, "floor(1.75) == 1");

  /* fixedpt_ceil */
  printf("--- fixedpt_ceil ---\n");
  CHECK(fixedpt_ceil(FIXEDPT_ONE * 3 / 2) == FIXEDPT_ONE * 2, "ceil(1.5) == 2");
  CHECK(fixedpt_ceil(FIXEDPT_ONE * 2) == FIXEDPT_ONE * 2, "ceil(2.0) == 2");
  CHECK(fixedpt_ceil(FIXEDPT_ONE * -3 / 2) == FIXEDPT_ONE * -1, "ceil(-1.5) == -1");
  CHECK(fixedpt_ceil(FIXEDPT_ONE * -2) == FIXEDPT_ONE * -2, "ceil(-2.0) == -2");
  CHECK(fixedpt_ceil(0) == 0, "ceil(0) == 0");
  CHECK(fixedpt_ceil(FIXEDPT_ONE * 7 / 4) == FIXEDPT_ONE * 2, "ceil(1.75) == 2");

  printf("\n=== result: %d passed, %d failed ===\n", passed, failed);
  return failed > 0 ? 1 : 0;
}

all:
	:

check:
	@pio check --environment $(env) \
		--src-filters="+<src/$(env)/**> +<src/main-$(env).cpp>" \
		--skip-packages

run-tests:
	@pio test -v

run-tests-native:
	@pio test --environment native -vvv

run-unit-tests-native:
	@pio test --filter '*unit$(filt)*' --environment native -vvv

run-fuzz-tests-native:
	@pio test --filter '*fuzz$(filt)*' --environment native -vvv

run-tests-nano:
	@pio test --environment nano -v

run-unit-tests-nano:
	@pio test --filter '*unit$(filt)*' --environment nano -vvv

run-fuzz-tests-nano:
	@pio test --filter '*fuzz$(filt)*' --environment nano -vvv

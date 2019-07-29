PARTFILES=\
	  part.14segment_red_f4c4fce2da510f7b1cb0a57d16014bbf_8.fzp \
	  part.adafruit_2649d5b78ee2dd9f6366251ff73ecbef_1.fzp \
	  svg.breadboard.14segment_red_c022aaca2c6efc1f284470ec769949f5_1_breadboard.svg \
	  svg.breadboard.adafruit_aef4889ce7f603b3a9419515e188601d_2_breadboard.svg \
	  svg.icon.14segment_red_c022aaca2c6efc1f284470ec769949f5_1_icon.svg \
	  svg.icon.adafruit_aef4889ce7f603b3a9419515e188601d_2_icon.svg \
	  svg.pcb.14segment_red_c022aaca2c6efc1f284470ec769949f5_1_pcb.svg \
	  svg.pcb.adafruit_aef4889ce7f603b3a9419515e188601d_2_pcb.svg \
	  svg.schematic.14segment_red_c022aaca2c6efc1f284470ec769949f5_1_schematic.svg \
	  svg.schematic.adafruit_aef4889ce7f603b3a9419515e188601d_2_schematic.svg

.PHONY: all clean prepare

all: timecircuit_display_row.fzz

clean:
	rm -f timecircuit_display_row.fzz

%.fzz: %.fz $(PARTFILES)
	zip $@ $?

prepare:
	unzip -o timecircuit_display_row.fzz
	perl cleanup_pre_commit.pl timecircuit_display_row.fz > timecircuit_display_row.fz.new
	perl cleanup_pre_commit.pl bttf_keypad.fz > bttf_keypad.fz.new
	mv -f timecircuit_display_row.fz.new timecircuit_display_row.fz
	mv -f bttf_keypad.fz.new bttf_keypad.fz


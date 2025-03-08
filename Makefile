SHELL := /bin/sh
CXXFLAGS := -Os -pipe -fno-exceptions -fno-rtti
LDFLAGS := -Wl,-O1

ifneq ($(shell fltk-config --ldflags | grep -q Xft && echo yes),)
	CPPFLAGS += -DHAVE_XFT
endif

CXXFLAGS += $(shell fltk-config --cxxflags) -Wall -ffunction-sections -fdata-sections -Wno-strict-aliasing -lX11
LDFLAGS += $(shell fltk-config --ldflags --use-images) -Wl,-gc-sections

all: flwm flwm_topside

flwm:
	$(CXX) -o $@ $(wildcard *.C) $(CXXFLAGS) $(LDFLAGS) $(CPPFLAGS)
flwm_topside:
	$(CXX) -o $@ $(wildcard *.C) -DTOPSIDE $(CXXFLAGS) $(LDFLAGS) $(CPPFLAGS)

install: flwm flwm_topside
ifndef INSTALL_PREFIX
	cp flwm flwm_topside "/usr/bin/"
else 
	@echo "Using custom installation directory: $(INSTALL_PREFIX)"
	cp flwm flwm_topside $(INSTALL_PREFIX)
endif
	

clean:
	rm -f flwm flwm_topside
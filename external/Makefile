GLM_VERSION = 0.9.9.8

.PHONY: all
all: glm

glm:
	curl -Lo glm.tmp.zip https://github.com/g-truc/glm/releases/download/$(GLM_VERSION)/glm-$(GLM_VERSION).zip
	unzip -d . glm.tmp.zip
	rm glm.tmp.zip

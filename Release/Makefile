VERSION=1.7.6
BETA=FALSE
SRC_DIR=..
BUILD_DIR=build
RELEASE_NAME=A52Codec
ADIUM_DIR=$(BUILD_DIR)/A52Codec

ifeq ($(BETA),TRUE)
    BUILDSTYLE=Development
else
    BUILDSTYLE=Deployment
endif

ifeq ($(strip $(PRODUCT_DIR)),)
	ADIUM_BUILD_DIR=$(SRC_DIR)/build/$(BUILDSTYLE)
else
	TARGET_BUILD_DIR=$(PRODUCT_DIR)/$(BUILDSTYLE)
	ADIUM_BUILD_DIR=$(TARGET_BUILD_DIR)
endif

release: createfolder diskimage

createfolder:
	@# clean build directory
	rm -rf $(BUILD_DIR)
	mkdir $(BUILD_DIR)

	mkdir $(ADIUM_DIR)
	
	@# copy the app
	cp -R $(ADIUM_BUILD_DIR)/A52Codec.component $(ADIUM_DIR)
	cp -R $(ADIUM_BUILD_DIR)/AC3MovieImport.component $(ADIUM_DIR)
	cp -R $(ADIUM_BUILD_DIR)/A52Preferences.app $(ADIUM_DIR)

	@# copy the text files
#	cp $(SRC_DIR)/Changes.txt $(ADIUM_DIR)
#	cp $(SRC_DIR)/License.txt $(ADIUM_DIR)
	cp $(SRC_DIR)/Read\ Me.rtf $(ADIUM_DIR)
	cp $(SRC_DIR)/Copying.txt $(ADIUM_DIR)
	
	@# symlink /Library/QuickTime
	ln -s /Library/QuickTime $(ADIUM_DIR)/QuickTime
	ln -s /Library/Audio/Plug-Ins/Components $(ADIUM_DIR)/Components

	@# delete svn  and backup files
	find $(BUILD_DIR) -name ".svn" -type d -exec rm -rf {} \; -prune
	find $(BUILD_DIR) \( -name "*~" -or -name .DS_Store \) -type f -delete
	
	@# delete unneeded nib files for non-beta builds
ifeq ($(BETA),FALSE)
	find $(BUILD_DIR) \( -name classes.nib -or -name info.nib \) -type f -delete
endif

diskimage:
	rm "$(BUILD_DIR)/$(RELEASE_NAME).dmg" || true
	@# make Adium disk image
	mkdir -p $(ADIUM_DIR)/.background
	cp dmgBackground.png $(ADIUM_DIR)/.background

	./make-diskimage.sh $(BUILD_DIR)/$(RELEASE_NAME)_$(VERSION).dmg $(ADIUM_DIR) "A52Codec $(VERSION)" dmg_adium.scpt
	
	@echo Build finished

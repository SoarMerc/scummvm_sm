/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define TRANSLATIONS_DAT_VER 4

#include "common/translation.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/unicode-bidi.h"

#ifdef USE_TRANSLATION

namespace Common {

DECLARE_SINGLETON(MainTranslationManager);

bool operator<(const TLanguage &l, const TLanguage &r) {
	return l.name < r.name;
}

TranslationManager::TranslationManager(const Common::String &fileName) : _currentLang(-1) {
	loadTranslationsInfoDat(fileName);

	// Set the default language
	setLanguage("");
}

TranslationManager::~TranslationManager() {

}

int32 TranslationManager::findMatchingLanguage(const String &lang) {
	uint langLength = lang.size();
	uint numLangs = _langs.size();

	// Try to match languages of the same length or longer ones
	// that can be cut at the length of the given one.
	for (uint i = 0; i < numLangs; ++i) {
		uint iLength = _langs[i].size();
		if (iLength >= langLength) {
			// Found a candidate; compare the full string by default.
			String cmpLang = _langs[i];

			if ((iLength > langLength) && (_langs[i][langLength] == '_')) {
				// It has a separation mark at the length of the
				// requested language, so we can cut it.
				cmpLang = String(_langs[i].c_str(), langLength);
			}
			if (lang.equalsIgnoreCase(cmpLang))
				return i;
		}
	}

	// Couldn't find a matching language.
	return -1;
}

void TranslationManager::setLanguage(const String &lang) {
	// Get lang index.
	int langIndex = -1;
	String langStr(lang);
	if (langStr.empty())
		langStr = g_system->getSystemLanguage();

	// Search for the given language or a variant of it.
	langIndex = findMatchingLanguage(langStr);

	// Try to find a partial match taking away parts of the original language.
	const char *lastSep;
	String langCut(langStr);
	while ((langIndex == -1) && (lastSep = strrchr(langCut.c_str(), '_'))) {
		langCut = String(langCut.c_str(), lastSep);
		langIndex = findMatchingLanguage(langCut);
	}

	// Load messages for that language.
	// Call it even if the index is -1 to unload previously loaded translations.
	if (langIndex != _currentLang) {
		loadLanguageDat(langIndex);
		_currentLang = langIndex;
	}
}

U32String TranslationManager::getTranslation(const char *message) const {
	return getTranslation(message, nullptr);
}

U32String TranslationManager::getTranslation(const char *message, const char *context) const {
	// If no language is set or message is empty, return msgid as is
	if (_currentTranslationMessages.empty() || *message == '\0')
		return U32String(message);

	// Binary-search for the msgid
	int leftIndex = 0;
	int rightIndex = _currentTranslationMessages.size() - 1;

	while (rightIndex >= leftIndex) {
		const int midIndex = (leftIndex + rightIndex) / 2;
		const PoMessageEntry *const m = &_currentTranslationMessages[midIndex];

		int compareResult = strcmp(message, _messageIds[m->msgid].c_str());

		if (compareResult == 0) {
			// Get the range of messages with the same ID (but different context)
			leftIndex = rightIndex = midIndex;
			while (
			    leftIndex > 0 &&
			    _currentTranslationMessages[leftIndex - 1].msgid == m->msgid
			) {
				--leftIndex;
			}
			while (
			    rightIndex < (int)_currentTranslationMessages.size() - 1 &&
			    _currentTranslationMessages[rightIndex + 1].msgid == m->msgid
			) {
				++rightIndex;
			}
			// Find the context we want
			if (context == nullptr || *context == '\0' || leftIndex == rightIndex)
				return _currentTranslationMessages[leftIndex].msgstr.decode();
			// We could use again binary search, but there should be only a small number of contexts.
			while (rightIndex > leftIndex) {
				compareResult = strcmp(context, _currentTranslationMessages[rightIndex].msgctxt.c_str());
				if (compareResult == 0)
					return _currentTranslationMessages[rightIndex].msgstr.decode();
				else if (compareResult > 0)
					break;
				--rightIndex;
			}
			return _currentTranslationMessages[leftIndex].msgstr.decode();
		} else if (compareResult < 0)
			rightIndex = midIndex - 1;
		else
			leftIndex = midIndex + 1;
	}

	return U32String(message);
}

String TranslationManager::getCurrentLanguage() const {
	if (_currentLang == -1)
		return "en";
	return _langs[_currentLang];
}

bool TranslationManager::currentIsBuiltinLanguage() const {
	return (_currentLang == -1);
}

U32String TranslationManager::getTranslation(const String &message) const {
	return getTranslation(message.c_str());
}

U32String TranslationManager::getTranslation(const String &message, const String &context) const {
	return getTranslation(message.c_str(), context.c_str());
}

const TLangArray TranslationManager::getSupportedLanguageNames() const {
	TLangArray languages;

	for (unsigned int i = 0; i < _langNames.size(); i++) {
		TLanguage lng(_langNames[i].decode(), i + 1);
		languages.push_back(lng);
	}

	sort(languages.begin(), languages.end());

	return languages;
}

int TranslationManager::parseLanguage(const String &lang) const {
	for (unsigned int i = 0; i < _langs.size(); i++) {
		if (lang == _langs[i])
			return i + 1;
	}

	return kTranslationBuiltinId;
}

String TranslationManager::getLangById(int id) const {
	switch (id) {
	case kTranslationAutodetectId:
		return "";
	case kTranslationBuiltinId:
		return "en";
	default:
		if (id >= 0 && id - 1 < (int)_langs.size())
			return _langs[id - 1];
	}

	// In case an invalid ID was specified, we will output a warning
	// and return the same value as the auto detection id.
	warning("Invalid language id %d passed to TranslationManager::getLangById", id);
	return "";
}

bool TranslationManager::openTranslationsFile(File &inFile) {
	// First look in the Themepath if we can find the file.
	if (ConfMan.hasKey("themepath") && openTranslationsFile(FSNode(ConfMan.getPath("themepath")), inFile))
		return true;

	// Then try to open it using the SearchMan.
	ArchiveMemberList fileList;
	SearchMan.listMatchingMembers(fileList, Common::Path(_translationsFileName, Common::Path::kNoSeparator));
	for (auto &m : fileList) {
		SeekableReadStream *const stream = m->createReadStream();
		if (stream && inFile.open(stream, m->getName())) {
			if (checkHeader(inFile))
				return true;
			inFile.close();
		}
	}

	return false;
}

bool TranslationManager::openTranslationsFile(const FSNode &node, File &inFile, int depth) {
	if (!node.exists() || !node.isReadable() || !node.isDirectory())
		return false;

	// Check if we can find the file in this directory
	// Since File::open(FSNode) makes all the needed tests, it is not really
	// necessary to make them here. But it avoid printing warnings.
	FSNode fileNode = node.getChild(_translationsFileName);
	if (fileNode.exists() && fileNode.isReadable() && !fileNode.isDirectory()) {
		if (inFile.open(fileNode)) {
			if (checkHeader(inFile))
				return true;
			inFile.close();
		}
	}

	// Check if we exceeded the given recursion depth
	if (depth - 1 == -1)
		return false;

	// Otherwise look for it in sub-directories
	FSList fileList;
	if (!node.getChildren(fileList, FSNode::kListDirectoriesOnly))
		return false;

	for (auto &file : fileList) {
		if (openTranslationsFile(file, inFile, depth == -1 ? - 1 : depth - 1))
			return true;
	}

	// Not found in this directory or its sub-directories
	return false;
}

void TranslationManager::loadTranslationsInfoDat(const Common::String &name) {
	File in;
	_translationsFileName = name;
	if (!openTranslationsFile(in)) {
		warning("You are missing a valid '%s' file. GUI translation will not be available", name.c_str());
		return;
	}

	char buf[256];
	int len;

	// Get number of translations
	int nbTranslations = in.readUint16BE();

	// Skip translation description & size for the original language (english) block
	// Also skip size of each translation block. Each block is written in Uint32BE.
	for (int i = 0; i < nbTranslations + 2; i++) {
		in.readUint32BE();
	}

	// Read list of languages
	_langs.resize(nbTranslations);
	_langNames.resize(nbTranslations);
	for (int i = 0; i < nbTranslations; ++i) {
		len = in.readUint16BE();
		in.read(buf, len);
		_langs[i] = String(buf, len - 1);
		len = in.readUint16BE();
		in.read(buf, len);
		_langNames[i] = String(buf, len - 1);
	}

	// Read messages
	int numMessages = in.readUint16BE();
	_messageIds.resize(numMessages);
	for (int i = 0; i < numMessages; ++i) {
		len = in.readUint16BE();
		String msg;
		while (len > 0) {
			in.read(buf, len > 256 ? 256 : len);
			msg += String(buf, len > 256 ? 256 : len - 1);
			len -= 256;
		}
		_messageIds[i] = msg;
	}
}

void TranslationManager::loadLanguageDat(int index) {
	_currentTranslationMessages.clear();
	// Sanity check
	if (index < 0 || index >= (int)_langs.size()) {
		if (index != -1)
			warning("Invalid language index %d passed to TranslationManager::loadLanguageDat", index);
		return;
	}

	File in;
	if (!openTranslationsFile(in))
		return;

	char buf[1024];
	int len;

	// Get number of translations
	int nbTranslations = in.readUint16BE();
	if (nbTranslations != (int)_langs.size()) {
		warning("The 'translations.dat' file has changed since starting ScummVM. GUI translation will not be available");
		return;
	}

	// Get size of blocks to skip.
	int skipSize = 0;

	// Skip translation description & size for the original language (english) block
	// Also skip size of each translation block. All block sizes are written in Uint32BE.
	for (int i = 0; i < index + 2; ++i)
		skipSize += in.readUint32BE();

	// We also need to skip the remaining block sizes
	skipSize += 4 * (nbTranslations - index);	// 4 because block sizes are written in Uint32BE in the .dat file.

	// Seek to start of block we want to read
	in.seek(skipSize, SEEK_CUR);

	// Read number of translated messages
	int nbMessages = in.readUint16BE();
	_currentTranslationMessages.resize(nbMessages);

	// Read messages
	for (int i = 0; i < nbMessages; ++i) {
		_currentTranslationMessages[i].msgid = in.readUint16BE();
		len = in.readUint16BE();
		String msg;
		while (len > 0) {
			in.read(buf, len > 256 ? 256 : len);
			msg += String(buf, len > 256 ? 256 : len - 1);
			len -= 256;
		}
		_currentTranslationMessages[i].msgstr = msg;
		len = in.readUint16BE();
		if (len > 0) {
			in.read(buf, len);
			_currentTranslationMessages[i].msgctxt = String(buf, len - 1);
		}
	}
}

bool TranslationManager::checkHeader(File &in) {
	char buf[13];
	int ver;

	in.read(buf, 12);
	buf[12] = '\0';

	// Check header
	if (strcmp(buf, "TRANSLATIONS") != 0) {
		warning("File '%s' is not a valid translations data file. Skipping this file", in.getName());
		return false;
	}

	// Check version
	ver = in.readByte();

	if (ver != TRANSLATIONS_DAT_VER) {
		warning("File '%s' has a mismatching version, expected was %d but you got %d. Skipping this file", in.getName(), TRANSLATIONS_DAT_VER, ver);
		return false;
	}

	return true;
}

} // End of namespace Common

#endif // USE_TRANSLATION

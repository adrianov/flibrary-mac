namespace HomeCompa::Global
{

constexpr auto COVER    = "cover";
constexpr auto IMAGE    = "image";
constexpr auto COVERS   = "covers";
constexpr auto IMAGES   = "images";
constexpr auto PICTURES = "pictures";

constexpr int APP_FAILED  = 1;
constexpr int RESTART_APP = 1234;

const auto INFO = [] {
	static constexpr char32_t info = 0x0001F6C8;
	return QString::fromUcs4(&info, 1);
}();

constexpr auto FONT_KEY      = "ui/Font";
constexpr auto FONT_SIZE_KEY = "ui/Font/pointSizeF";
#ifdef __APPLE__
constexpr auto FONT_SIZE_DEFAULT = 13;
#else
constexpr auto FONT_SIZE_DEFAULT = 9;
#endif

}

namespace HomeCompa::Preferences
{

constexpr auto PREFER_HIDE_SCROLLBARS_KEY = "Preferences/hideScrollBars";

}

namespace HomeCompa::Export
{

constexpr auto GRAYSCALE_COVER_KEY  = "ui/Export/GrayscaleCover";
constexpr auto GRAYSCALE_IMAGES_KEY = "ui/Export/GrayscaleImages";
constexpr auto REMOVE_COVER_KEY     = "ui/Export/RemoveCover";
constexpr auto REMOVE_IMAGES_KEY    = "ui/Export/RemoveImages";
constexpr auto CONVERT_IMAGES_KEY   = "ui/Export/ConvertImagesToJpegPng";

}

namespace HomeCompa::Inpx
{

constexpr auto STRUCTURE_INFO                     = "structure.info";
constexpr auto VERSION_INFO                       = "version.info";
constexpr auto COLLECTION_INFO                    = "collection.info";
constexpr auto REVIEWS_FOLDER                     = "reviews";
constexpr auto REVIEWS_ADDITIONAL_ARCHIVE_NAME    = "additional.zip";
constexpr auto REVIEWS_ADDITIONAL_BOOKS_FILE_NAME = "books.json";
constexpr auto COMPILATIONS                       = "compilations.7z";
constexpr auto ANNOTATIONS                        = "annotations.7z";
constexpr auto CONTENTS                           = "contents.7z";
constexpr auto COMPILATIONS_JSON                  = "compilations.json";
constexpr auto AUTHORS_FOLDER                     = "authors";
constexpr auto INP_FIELDS_DESCRIPTION             = "AUTHOR;GENRE;TITLE;SERIES;SERNO;FILE;SIZE;LIBID;DEL;EXT;DATE;INSNO;LANG;LIBRATE;KEYWORDS;YEAR;SOURCELIB;";

constexpr auto TIME = "time";
constexpr auto NAME = "name";
constexpr auto TEXT = "text";

constexpr auto FOLDER = "folder";
constexpr auto FILE   = "file";
constexpr auto SUM    = "sum";
constexpr auto COUNT  = "count";

constexpr auto COMPILATION = "compilation";
constexpr auto COVERED     = "covered";
constexpr auto PART        = "part";

} // namespace HomeCompa::Inpx

namespace HomeCompa::Epub
{

constexpr std::string_view CONTAINER_FILE_NAME = "META-INF/container.xml";

constexpr auto IMAGE_INDEX_FILE_NAME = "FLibraryImageIndex.json";
constexpr auto IMAGE_INDEX_ID        = "id";
constexpr auto IMAGE_INDEX_NUM       = "num";

}

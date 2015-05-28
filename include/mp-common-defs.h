#ifndef __MP_COMMON_DEFS_H__
#define __MP_COMMON_DEFS_H__

#define MP_B_PATH "path"
#define MP_MM_KEY "multimedia_key"
#define MP_PLAY_RECENT "play_recent"
#define MP_REQ_TYPE "request_type"

//ug-music-player request type
#define UG_MP_REQ_SELECT_SINGLE	"Select"
#define UG_MP_REQ_SELECT_MULTI		"MultipleSelect"

//for shorcut
#define MP_FUNC_ADD_TO_HOME_SEPARATION		"/"
#define MP_FUNC_ADD_TO_HOME_DESC_PREFIX	"_Shortcut:Description://"
#define MP_FUNC_ADD_TO_HOME_THUMBNAIL_PREFIX	"_Shortcut:ThumbnailPath://"
#define MP_FUNC_ADD_TO_HOME_PREFIX 		"_Shortcut:MusicPlayer://"
#define MP_SHORTCUT_PLAYLIST				"playlist/"
#define MP_SHORTCUT_GROUP					"group/"
#define MP_SHORTCUT_SONG					"song/"
#define MP_FUNC_ADD_TO_HOME_PLAYLIST		MP_FUNC_ADD_TO_HOME_PREFIX"playlist/"
#define MP_FUNC_ADD_TO_HOME_GROUP		MP_FUNC_ADD_TO_HOME_PREFIX"group/"
#define MP_FUNC_ADD_TO_HOME_SONG			MP_FUNC_ADD_TO_HOME_PREFIX"song/"

#endif

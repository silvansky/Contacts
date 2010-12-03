#ifndef DEF_ROSTERINDEXTYPEROLE_H
#define DEF_ROSTERINDEXTYPEROLE_H

enum RosterIndexTypes
{
	RIT_ANY_TYPE,
	RIT_ROOT,
	RIT_STREAM_ROOT,
	RIT_GROUP_BLANK,
	RIT_CONTACT,
	RIT_GROUP,
	RIT_GROUP_NOT_IN_ROSTER,
	RIT_GROUP_MY_RESOURCES,
	RIT_GROUP_AGENTS,
	RIT_AGENT,
	RIT_MY_RESOURCE,
	//RosterSearch
	RIT_SEARCH_EMPTY,
	RIT_SEARCH_LINK,
	//MetaContacts
	RIT_METACONTACT
};

enum RosterIndexDataRoles
{
	RDR_ANY_ROLE = 32,
	RDR_TYPE,
	RDR_INDEX_ID,
	//XMPP Roles
	RDR_STREAM_JID,
	RDR_JID,
	RDR_PJID,
	RDR_BARE_JID,
	RDR_NAME,
	RDR_GROUP,
	RDR_SHOW,
	RDR_STATUS,
	RDR_PRIORITY,
	RDR_SUBSCRIBTION,
	RDR_ASK,
	//Decoration Roles
	RDR_FONT_HINT,
	RDR_FONT_SIZE,
	RDR_FONT_WEIGHT,
	RDR_FONT_STYLE,
	RDR_FONT_UNDERLINE,
	//Labels roles
	RDR_LABEL_ITEMS,
	RDR_DISPLAY_FLAGS,
	RDR_DECORATION_FLAGS,
	RDR_FOOTER_TEXT,
	//Avatars
	RDR_AVATAR_HASH,
	RDR_AVATAR_IMAGE,
	//Annotations
	RDR_ANNOTATIONS,
	//Search
	RDR_SEARCH_LINK,
	//DND
	RDR_IS_DRAGGED,
	//Mouse cursor
	RDR_MOUSE_CURSOR,
	//Visibility
	RDR_ALLWAYS_VISIBLE,
	RDR_ALLWAYS_INVISIBLE,
	//IndexStates
	RDR_STATES_FORCE_ON,
	RDR_STATES_FORCE_OFF,
	// Group Labels
	RDR_GROUP_COUNTER
};

#endif
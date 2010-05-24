#ifndef DEF_ROSTERINDEXTYPEROLE_H
#define DEF_ROSTERINDEXTYPEROLE_H

/**
* @brief типы элементов ростера
* @enum RosterIndexTypes
*/
enum RosterIndexTypes
{
	/**
	* @brief любой тип
	* @var RIT_ANY_TYPE
	*/
	RIT_ANY_TYPE,
	/**
	* @brief корневой элемент
	* @var RIT_ROOT
	*/
	RIT_ROOT,
	/**
	* @brief корневой элемент потока
	* @var RIT_STREAM_ROOT
	*/
	RIT_STREAM_ROOT,
	/**
	* @brief основная группа
	* @var RIT_GROUP_BLANK
	*/
	RIT_GROUP_BLANK,
	/**
	* @brief группа
	* @var RIT_GROUP
	*/
	RIT_CONTACT,
	/**
	* @brief агент (транспорт)
	* @var RIT_AGENT
	*/
	RIT_GROUP,
	/**
	* @brief группа "не в списке"
	* @var RIT_GROUP_NOT_IN_ROSTER
	*/
	RIT_GROUP_NOT_IN_ROSTER,
	/**
	* @brief группа "мои ресурсы"
	* @var RIT_GROUP_MY_RESOURCES
	*/
	RIT_GROUP_MY_RESOURCES,
	/**
	* @brief группа "агенты" (транспорты)
	* @var RIT_GROUP_AGENTS
	*/
	RIT_GROUP_AGENTS,
	/**
	* @brief контакт
	* @var RIT_CONTACT
	*/
	RIT_AGENT,
	/**
	* @brief мой ресурс
	* @var RIT_MY_RESOURCE
	*/
	RIT_MY_RESOURCE,
	/**
	* @brief заглушка "Контакты не найдены"
	* @var RIT_SEARCH_EMPTY
	*/
	RIT_SEARCH_EMPTY,
	/**
	* @brief ссылка на поиск
	* @var RIT_SEARCH_LINK
	*/
	RIT_SEARCH_LINK
};

/**
* @brief типы данных элементов ростера
* @enum RosterIndexDataRoles
*/
enum RosterIndexDataRoles
{
	/**
	* @brief любая роль
	* @var RDR_ANY_ROLE
	*/
	RDR_ANY_ROLE = 32,
	/**
	* @brief тип элемента
	* @var RDR_TYPE
	*/
	RDR_TYPE,
	/**
	* @brief индекс
	* @var RDR_INDEX_ID
	*/
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
	RDR_LABEL_ID,
	RDR_LABEL_ORDERS,
	RDR_LABEL_VALUES,
	RDR_LABEL_FLAGS,
	RDR_FOOTER_TEXT,
	//Avatars
	RDR_AVATAR_HASH,
	RDR_AVATAR_IMAGE,
	//Annotations
	RDR_ANNOTATIONS,
	//Search
	RDR_SEARCH_LINK,
	RDR_SEARCH_CAPTION,
	//DND
	RDR_IS_DRAGGED,
	//Mouse cursor
	RDR_MOUSE_CURSOR
};

#endif

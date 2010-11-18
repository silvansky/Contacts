#ifndef IBIRTHDAYREMINDER_H
#define IBIRTHDAYREMINDER_H

#define BIRTHDAYREMINDER_UUID "78E8CA6C-99F0-4F38-B52B-3FF522B7F110}"

class IBirthdayReminder
{
public:
	virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IBirthdayReminder,"Virtus.Plugin.IBirthdayReminer/1.0")

#endif

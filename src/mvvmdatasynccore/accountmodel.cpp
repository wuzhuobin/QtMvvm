#include "accountmodel.h"
#include "accountmodel_p.h"
#include "datasyncviewmodel.h"

#include <QRemoteObjectReplica>

using namespace QtMvvm;
using namespace QtDataSync;

AccountModel::AccountModel(QObject *parent) :
	QAbstractListModel(parent),
	d(new AccountModelPrivate())
{}

AccountModel::~AccountModel() {}

void AccountModel::setup(AccountManager *accountManager, SyncManager *syncManager)
{
	beginResetModel();
	d->devices.clear();
	if(d->accountManager)
		d->accountManager->disconnect(this);
	d->accountManager = accountManager;
	if(d->syncManager)
		d->syncManager->disconnect(this);
	d->syncManager = syncManager;
	endResetModel();

	connect(d->accountManager, &AccountManager::accountDevices,
			this, &AccountModel::accountDevices);
	connect(d->accountManager, &AccountManager::importAccepted,
			this, &AccountModel::reload);
	connect(d->accountManager, &AccountManager::accountAccessGranted,
			this, &AccountModel::reload);

	if(d->accountManager->replica()->isInitialized())
		d->accountManager->listDevices();
	else {
		connect(d->accountManager->replica(), &QRemoteObjectReplica::initialized,
				d->accountManager, &AccountManager::listDevices);
	}

	connect(d->syncManager, &SyncManager::syncStateChanged,
			this, &AccountModel::update);
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation != Qt::Horizontal)
		return {};

	switch (section) {
	case 0:
		switch(role) {
		case NameRole:
			return tr("Name");
		case FingerPrintRole:
			return tr("Fingerprint");
		default:
			break;
		}
		break;
	case 1:
		if(role == Qt::DisplayRole)
			return tr("Fingerprint");
		break;
	default:
		break;
	}

	return {};
}

int AccountModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return d->devices.size();
}

int AccountModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return 2;
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (index.column()) {
	case 0:
		switch(role) {
		case NameRole:
			return d->devices.value(index.row()).name();
		case FingerPrintRole:
			return DataSyncViewModel::formatFingerPrint(d->devices.value(index.row()).fingerprint());
		default:
			break;
		}
		break;
	case 1:
		if(role == Qt::DisplayRole)
			return DataSyncViewModel::formatFingerPrint(d->devices.value(index.row()).fingerprint());
		break;
	default:
		break;
	}

	return {};
}

bool AccountModel::removeDevice(const QModelIndex &index)
{
	if (!index.isValid())
		return false;
	else {
		d->accountManager->removeDevice(d->devices.value(index.row()));
		return true;
	}
}

void AccountModel::reload()
{
	beginResetModel();
	d->devices.clear();
	endResetModel();

	if(d->accountManager)
		d->accountManager->listDevices();
}

void AccountModel::accountDevices(const QList<DeviceInfo> &devices)
{
	beginResetModel();
	d->devices = devices;
	endResetModel();
}

void AccountModel::update(SyncManager::SyncState state)
{
	if(state == SyncManager::Disconnected ||
	   state == SyncManager::Error ||
	   state == SyncManager::Initializing)
		reload();
}

// ------------- Private Implementation -------------

AccountModelPrivate::AccountModelPrivate() :
	accountManager(nullptr),
	syncManager(nullptr),
	devices()
{}

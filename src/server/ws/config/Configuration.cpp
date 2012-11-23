/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "Configuration.h"
#include "common/error.h"

#include <set>

#include <boost/lexical_cast.hpp>

namespace fts3 {
namespace ws {

using namespace boost;

const string Configuration::Protocol::BANDWIDTH = "bandwidth";
const string Configuration::Protocol::NOSTREAMS = "nostreams";
const string Configuration::Protocol::TCP_BUFFER_SIZE = "tcp_buffer_size";
const string Configuration::Protocol::NOMINAL_THROUGHPUT = "nominal_throughput";
const string Configuration::Protocol::BLOCKSIZE = "blocksize";
const string Configuration::Protocol::HTTP_TO = "http_to";
const string Configuration::Protocol::URLCOPY_PUT_TO = "urlcopy_put_to";
const string Configuration::Protocol::URLCOPY_PUTDONE_TO = "urlcopy_putdone_to";
const string Configuration::Protocol::URLCOPY_GET_TO = "urlcopy_get_to";
const string Configuration::Protocol::URLCOPY_GET_DONETO = "urlcopy_get_doneto";
const string Configuration::Protocol::URLCOPY_TX_TO = "urlcopy_tx_to";
const string Configuration::Protocol::URLCOPY_TXMARKS_TO = "urlcopy_txmarks_to";
const string Configuration::Protocol::URLCOPY_FIRST_TXMARK_TO = "urlcopy_first_txmark_to";
const string Configuration::Protocol::TX_TO_PER_MB = "tx_to_per_mb";
const string Configuration::Protocol::NO_TX_ACTIVITY_TO = "no_tx_activity_to";
const string Configuration::Protocol::PREPARING_FILES_RATIO = "preparing_files_ratio";

const string Configuration::any = "*";
const string Configuration::wildcard = "(*)";
const string Configuration::on = "on";
const string Configuration::off = "off";

Configuration::Configuration(string dn) :
		dn(dn),
		db (DBSingleton::instance().getDBObjectInstance()),
		insertCount(0),
		updateCount(0),
		deleteCount(0) {

	notAllowed.insert(wildcard);

}

Configuration::~Configuration() {

	if (deleteCount)
		db->auditConfiguration(dn, all, "delete (x" + lexical_cast<string>(deleteCount) + ")");

	if (insertCount)
		db->auditConfiguration(dn, all, "insert (x" + lexical_cast<string>(insertCount) + ")");

	if (updateCount)
		db->auditConfiguration(dn, all, "update (x" + lexical_cast<string>(updateCount) + ")");
}

string Configuration::json(map<string, int> params) {

	stringstream ss;

	ss << "[";

	map<string, int>::iterator it;
	for (it = params.begin(); it != params.end();) {
		ss << "{\"" << it->first << "\":" << it->second << "}";
		it++;
		if (it != params.end()) ss << ",";
	}

	ss << "]";

	return ss.str();
}

string Configuration::json(vector<string> members) {

	stringstream ss;

	ss << "[";

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end();) {
		ss << "\"" << *it << "\"";
		it++;
		if (it != members.end()) ss << ",";
	}

	ss << "]";

	return ss.str();
}

void Configuration::addSe(string se, bool active) {
	//check if SE exists
	Se* ptr = 0;
	db->getSe(ptr, se);
	if (!ptr) {
		// if not add it to the DB
		db->addSe(string(), string(), string(), se, active ? on : off, string(), string(), string(), string(), string(), string());
		insertCount++;
	} else
		db->updateSe(string(), string(), string(), se, active ? on : off, string(), string(), string(), string(), string(), string());
		delete ptr;
}

void Configuration::eraseSe(string se) {
	db->updateSe(string(), string(), string(), se, on, string(), string(), string(), string(), string(), string());
	updateCount++;
}

void Configuration::addGroup(string group, vector<string>& members) {

	if (db->checkGroupExists(group)) {
		// if the group exists remove it!
		vector<string> tmp;
		db->getGroupMembers(group, tmp);
		db->deleteMembersFromGroup(group, tmp);
		deleteCount++;
	}

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		addSe(*it);
		if (db->checkIfSeIsMemberOfAnotherGroup(*it))
			throw Err_Custom("The SE: " + *it + " is already a member of another SE group!");
	}

	db->addMemberToGroup(group, members);
	insertCount++;
}

void Configuration::checkGroup(string group) {
	// check if the group exists
	if (!db->checkGroupExists(group)) {
		throw Err_Custom(
				"The group: " +  group + " does not exist!"
			);
	}
}

void Configuration::addLinkCfg(string source, string destination, bool active, string symbolic_name, map<string, int>& protocol) {

	scoped_ptr< pair<string, string> > p (
			db->getSourceAndDestination(symbolic_name)
		);

	if (p.get()) {
		if (source != p->first || destination != p->second)
			throw Err_Custom("A 'pair' with the same symbolic name exists already!");
	}

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	bool update = true;
	if (!cfg.get()) {
		cfg.reset(new LinkConfig);
		update = false;
	}

	cfg->source = source;
	cfg->destination = destination;
	cfg->state = active ? on : off;
	cfg->symbolic_name = symbolic_name;

	cfg->NOSTREAMS = protocol[Protocol::NOSTREAMS];
	cfg->TCP_BUFFER_SIZE = protocol[Protocol::TCP_BUFFER_SIZE];
	cfg->URLCOPY_TX_TO = protocol[Protocol::URLCOPY_TX_TO];
	cfg->NO_TX_ACTIVITY_TO = protocol[Protocol::NO_TX_ACTIVITY_TO];

	if (update) {
		db->updateLinkConfig(cfg.get());
		updateCount++;
	} else {
		db->addLinkConfig(cfg.get());
		insertCount++;
	}

}

void Configuration::addShareCfg(string source, string destination, map<string, int>& share) {
	// set with VOs that need an update
	set<string> update;
	// find all share configuration for source and destination
	vector<ShareConfig*> vec = db->getShareConfig(source, destination);
	vector<ShareConfig*>::iterator iv;
	// loop over share configuration
	for (iv = vec.begin(); iv != vec.end(); iv++) {
		scoped_ptr<ShareConfig> cfg (*iv);
		if (share.find(cfg->vo) == share.end()) {
			// if the VO was not in the new configuration remove the record
			db->deleteShareConfig(source, destination, cfg->vo);
			deleteCount++;
		} else {
			// otherwise schedule it for update
			update.insert(cfg->vo);
		}
	}
	// save the configuration in DB
	map<string, int>::iterator it;
	for (it = share.begin(); it != share.end(); it++) {
		// create new share configuration
		scoped_ptr<ShareConfig> cfg(new ShareConfig);
		cfg->source = source;
		cfg->destination = destination;
		cfg->vo = it->first;
		cfg->active_transfers = it->second;
		// check if the configuration should use insert or update
		if (update.count(it->first)) {
			db->updateShareConfig(cfg.get());
			updateCount++;
		} else {
			db->addShareConfig(cfg.get());
			insertCount++;
		}
	}
}

map<string, int> Configuration::getProtocolMap(string source, string destination) {

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	return getProtocolMap(cfg.get());
}

map<string, int> Configuration::getProtocolMap(LinkConfig* cfg) {

	map<string, int> ret;
	if (cfg->NOSTREAMS)
		ret[Protocol::NOSTREAMS] = cfg->NOSTREAMS;
	if (cfg->TCP_BUFFER_SIZE)
		ret[Protocol::TCP_BUFFER_SIZE] = cfg->TCP_BUFFER_SIZE;
	if (cfg->URLCOPY_TX_TO)
		ret[Protocol::URLCOPY_TX_TO] = cfg->URLCOPY_TX_TO;
	if (cfg->NO_TX_ACTIVITY_TO)
		ret[Protocol::NO_TX_ACTIVITY_TO] = cfg->NO_TX_ACTIVITY_TO;

	return ret;
}

map<string, int> Configuration::getShareMap(string source, string destination) {

	vector<ShareConfig*> vec = db->getShareConfig(source, destination);

	if (vec.empty()) {
		throw Err_Custom(
				"A configuration for source: '" + source + "' and destination: '" + destination + "' does not exist!"
			);
	}

	map<string, int> ret;

	vector<ShareConfig*>::iterator it;
	for (it = vec.begin(); it != vec.end(); it++) {
		scoped_ptr<ShareConfig> cfg(*it);
		ret[cfg->vo] = cfg->active_transfers;
	}

	return ret;
}

void Configuration::delLinkCfg(string source, string destination) {

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	if (!cfg.get())
		throw Err_Custom("A configuration for " + source + " - " + destination + " pair does not exist!");

	db->deleteLinkConfig(source, destination);
	deleteCount++;
}

void Configuration::delShareCfg(string source, string destination) {

	db->deleteShareConfig(source, destination);
	deleteCount++;
}

}
}

/****************************************************************************
**
**  This file is a part of Fahrplan.
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License along
**  with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "parser_london_tfl.h"

#include <QUrl>
#include <QNetworkReply>
#ifdef BUILD_FOR_QT5
#   include <QUrlQuery>
#else
#include <QTextDocument>
#   define setQuery(q) setQueryItems(q.queryItems())
#endif
#include <qmath.h>

namespace
{
   const QUrl BaseUrl("https://api.tfl.gov.uk");

   /*
   QString escapeHtmlChars(const QString & input)
   {

   }
   */
}

//QHash<QString, JourneyDetailResultList *> cachedJourneyDetailsTfl;

//inline qreal deg2rad(qreal deg)
//{
//    return deg * 3.141592653589793238463 / 180;
//}

/*
inline int distance(qreal lat1, qreal lon1, qreal lat2, qreal lon2)
{
    const qreal sdLat = qSin(deg2rad(lat2 - lat1) / 2);
    const qreal sdLon = qSin(deg2rad(lon2 - lon1) / 2);
    const qreal cLat1 = qCos(deg2rad(lat1));
    const qreal cLat2 = qCos(deg2rad(lat2));
    const qreal a = sdLat * sdLat + sdLon * sdLon * cLat1 * cLat2;
    const qreal c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));

    // 6371 km - Earth radius.
    // We return distance in meters, thus * 1000.
    return qRound(6371.0 * c * 1000);
}
*/

ParserLondonTfl::ParserLondonTfl(QObject *parent):ParserAbstract(parent)
{
    lastCoordinates.isValid = false;
}

void ParserLondonTfl::getTimeTableForStation(const Station &currentStation,
                                           const Station &,
                                           const QDateTime &,
                                           ParserAbstract::Mode,
                                           int trainrestrictions)
{
    QUrl relativeUri(QString("/locations/%1/departure-times").arg(currentStation.id.toString()));

#if defined(BUILD_FOR_QT5)
    QUrlQuery query;
#else
    QUrl query;
#endif

    query.addQueryItem("lang", 	"en-GB");
    relativeUri.setQuery(query);

    timetableRestrictions  =  trainrestrictions;
    sendHttpRequest(BaseUrl.resolved(relativeUri));
    currentRequestState=FahrplanNS::getTimeTableForStationRequest;
}

void ParserLondonTfl::findStationsByName(const QString &stationName)
{
    qDebug() << "FINDBYNAME";
    QUrl relativeUrl (QString("/Stoppoint/Search/%1").arg(stationName));

    QUrl uri(BaseUrl.resolved(relativeUrl));

    /*
#if defined(BUILD_FOR_QT5)
    QUrlQuery query;
#else
    QUrl query;
#endif
    query.addQueryItem("lang", 	"en-GB");
    query.addQueryItem("q", stationName);
    uri.setQuery(query);
    */

    sendHttpRequest(uri);
    currentRequestState=FahrplanNS::stationsByNameRequest;
}

void ParserLondonTfl::findStationsByCoordinates(qreal longitude, qreal latitude)
{
    return;
     
    QUrl uri(BaseUrl);
    //QUrl uri(BASE_URL "locations");

#if defined(BUILD_FOR_QT5)
    QUrlQuery query;
#else
    QUrl query;
#endif
    query.addQueryItem("lang", 	"en-GB");
    query.addQueryItem("type", "station,stop");
    query.addQueryItem("latlong", QString("%1,%2").arg(latitude).arg(longitude));
    query.addQueryItem("includestation", "true");
    uri.setQuery(query);

    lastCoordinates.isValid = true;
    lastCoordinates.latitude = latitude;
    lastCoordinates.longitude = longitude;
    currentRequestState = FahrplanNS::stationsByCoordinatesRequest;

    sendHttpRequest(uri);
}

void ParserLondonTfl::searchJourney(const Station &departureStation,
                                  const Station &viaStation,
                                  const Station &arrivalStation,
                                  const QDateTime &dateTime,
                                  const ParserAbstract::Mode mode,
                                  int trainrestrictions)
{
    lastsearch.from=departureStation;
    lastsearch.to=arrivalStation;
    lastsearch.restrictions=trainrestrictions;
    lastsearch.via=viaStation;

    QUrl relativeUri(QString("Journey/Journeyresults/%1/to/%2").arg(departureStation.id.toString(), arrivalStation.id.toString()));

#if defined(BUILD_FOR_QT5)
    QUrlQuery query;
#else
    QUrl query;
#endif

    /*
    query.addQueryItem("lang", 	"en-GB");
    query.addQueryItem("before", "1");
    query.addQueryItem("from", departureStation.id.toString());
    if(viaStation.valid)
        query.addQueryItem("via", viaStation.id.toString());
    query.addQueryItem("to", arrivalStation.id.toString());
    query.addQueryItem("sequence", "1");
    query.addQueryItem("dateTime", dateTime.toString("yyyy-MM-ddTHHmm"));

    switch(trainrestrictions){
    default:
    case all:
        query.addQueryItem("byFerry", "true");
    case noFerry:
        query.addQueryItem("byBus", "true");
        query.addQueryItem("byTram", "true");
        query.addQueryItem("bySubway", "true");
    case trainsOnly:
        query.addQueryItem("byTrain", "true");
    }
    query.addQueryItem("searchtype", mode==Departure?"departure":"arrival");

    //TODO: make transport types work
    query.addQueryItem("after", "5");

    uri.setQuery(query);
    sendHttpRequest(uri);

    */

    relativeUri.setQuery(query);
    sendHttpRequest(BaseUrl.resolved(relativeUri));

    currentRequestState=FahrplanNS::searchJourneyRequest;
}

void ParserLondonTfl::searchJourneyLater()
{
    searchJourney(lastsearch.from, lastsearch.via, lastsearch.to, lastsearch.lastOption, Departure , lastsearch.restrictions);

}

void ParserLondonTfl::searchJourneyEarlier()
{
    QDateTime time = lastsearch.firstOption.addSecs(-30*60);
    searchJourney(lastsearch.from, lastsearch.via, lastsearch.to,time, Departure, lastsearch.restrictions);
}

void ParserLondonTfl::getJourneyDetails(const QString &id)
{
    if(cachedResults.contains(id))
        emit journeyDetailsResult(cachedResults.value(id));
}

QStringList ParserLondonTfl::getTrainRestrictions()
{
 QStringList restrictions;
 restrictions << tr("All");
 restrictions << tr("Only trains");
 restrictions << tr("All, except ferry");
 return restrictions;
}

/*
bool sortByTimeLessThan(const TimetableEntry &first, const TimetableEntry &second)
{
    if (first.time == second.time)
        return first.trainType < second.trainType;
    else
        return first.time < second.time;
}
*/

void ParserLondonTfl::parseTimeTable(QNetworkReply *networkReply)
{
    return; 
    
    TimetableEntriesList result;
    QByteArray allData = networkReply->readAll();
//    qDebug() << "REPLY:>>>>>>>>>>>>\n" << allData;

    QVariantMap doc = parseJson(allData);
    if (doc.isEmpty()) {
        emit errorOccured(tr("Cannot parse reply from the server"));
        return;
    }

    QVariantList tabs = doc.value("tabs").toList();
    QString currentStation(doc.value("location").toMap().value("name").toString());

    QVariantList::const_iterator i;
    for (i = tabs.constBegin(); i != tabs.constEnd(); ++i) {
        QVariantMap tab = i->toMap();
        QString type = tab.value("id").toString();
        QVariantList departures = tab.value("departures").toList();
        switch(timetableRestrictions){
            case all:
            default:
            break;
        case trainsOnly:
            if(type!="train")
                continue;
            break;
        case noFerry:
            if(type=="ferry")//not sure if this ever happens
                continue;
            break;
        }

        QVariantList::const_iterator j;
        for (j = departures.constBegin(); j != departures.constEnd(); ++j) {
            QVariantMap departure = j->toMap();
            TimetableEntry entry;
            entry.currentStation=currentStation;
            entry.destinationStation = departure.value("destinationName").toString();
            entry.time = QTime::fromString(departure.value("time").toString(), "HH:mm");
            QString via(departure.value("viaNames").toString());
            if (!via.isEmpty())
                entry.destinationStation = tr("%1 via %2").arg(entry.destinationStation, via);
            QStringList info;
            const QString rtStatus = departure.value("realtimeState").toString();
            if (rtStatus == "ontime") {
                info << tr("On-Time");
            } else if (rtStatus == "late") {
                const QString rtMessage = departure.value("realtimeText").toString().trimmed();
                if (!rtMessage.isEmpty())
                    info << QString("<span style=\"color:#b30;\">%1</span>").arg(rtMessage);
            }
            const QString remark = departure.value("remark").toString();
            if (!remark.isEmpty())
                info << remark;
            entry.miscInfo = info.join("<br />");

            entry.platform = departure.value("platform").toString();

            QString train = departure.value("mode").toMap().value("name").toString();
            const QString service = departure.value("service").toString();
            if (!service.isEmpty())
                train.append(" ").append(service);
            entry.trainType = train;

            result.append(entry);
        }
    }

    // Departures / arrivals are grouped by transportation type,
    // while we want them sorted by departure / arrival time.
    //qSort(result.begin(), result.end(), sortByTimeLessThan);

    emit timetableResult(result);

}

void ParserLondonTfl::parseStationsByName(QNetworkReply *networkReply)
{
    qDebug() << "PARSING STATIONS";
    QByteArray allData = networkReply->readAll();
    qDebug() << "REPLY:>>>>>>>>>>>>\n" << allData;

    QVariantMap doc = parseJson(allData);
    if (doc.isEmpty()) {
        emit errorOccured(tr("Cannot parse reply from the server"));
        return;
    }

    QVariantList stations = doc.value("matches").toList();

    StationsList result;

    QVariantList::const_iterator i;
    for (i = stations.constBegin(); i != stations.constEnd(); ++i) {
        QVariantMap station = i->toMap();
        Station s;
        s.id = station.value("icsId");
        s.name = station.value("name").toString();
        s.latitude = station.value("lat").toDouble();
        s.longitude = station.value("lon").toDouble();
        s.miscInfo = "Zone " + station.value("zone").toString();
        result.append(s);
    }

    emit stationsResult(result);
}

void ParserLondonTfl::parseStationsByCoordinates(QNetworkReply *networkReply)
{
    return;
    
    parseStationsByName(networkReply);

    lastCoordinates.isValid = false;
}

void ParserLondonTfl::parseSearchJourney(QNetworkReply *networkReply)
{
    // baker street to great portland street
    //https://api.tfl.gov.uk/Journey/Journeyresults/1000011/to/1000091

    qDebug() << "PARSING JOURNEYS";
    QByteArray allData = networkReply->readAll();
    qDebug() << "REPLY:>>>>>>>>>>>>\n" << allData;

    QVariantMap doc = parseJson(allData);
    if (doc.isEmpty()) {
        emit errorOccured(tr("Cannot parse reply from the server"));
        return;
    }

    QVariantList journeys = doc.value("journeys").toList();

    JourneyResultList* result=new JourneyResultList;

    QDateTime arrival;
    QDateTime departure;

    QVariantList::const_iterator i;
    int counter = 0;

    for (i = journeys.constBegin(); i != journeys.constEnd(); ++i) {
        counter++;
        QVariantMap journey = i->toMap();
        QString id = QString::number(counter);
        parseJourneyOption(journey, id);
        JourneyResultItem* item = new JourneyResultItem;
        arrival = QDateTime::fromString(journey.value("arrivalDateTime").toString(), "yyyy-MM-ddTHH:mm:ss");
        departure = QDateTime::fromString(journey.value("startDateTime").toString(), "yyyy-MM-ddTHH:mm:ss");

        if (i == journeys.constBegin())
            lastsearch.firstOption=departure;

        item->setArrivalTime(arrival.toString("HH:mm"));
        item->setDepartureTime(departure.toString("HH:mm"));

        QVariantList legs = journey.value("legs").toList();
        QStringList trains;

        QVariantList::const_iterator j;
        for (j = legs.constBegin(); j != legs.constEnd(); ++j)
        {
            const QVariantMap mode = j->toMap().value("mode").toMap();
            if (mode.value("type").toString() != "walking") {
                const QString typeName = mode.value("name").toString();
                if (!typeName.isEmpty())
                    trains.append(typeName);
            }
        }

        trains.removeDuplicates();

        item->setTrainType(trains.join(", ").trimmed());

        //item->setTransfers(QString::number((int) journey.value("numberOfChanges").toDouble()));
        item->setTransfers(QString::number(legs.count() - 1));

        int minutes = departure.secsTo(arrival)/60;
        item->setDuration(QString("%1:%2").arg(minutes/60).arg(minutes%60,2,10,QChar('0')));

        //item->setId(journey.value("id").toString());
        //item->setId(randString(10));
        item->setId(id);

        result->appendItem(item);

        //Set result metadata based on first result
        if (result->itemcount() == 1) {
            result->setTimeInfo(arrival.date().toString());
            result->setDepartureStation(cachedResults[item->id()]->departureStation());
            result->setArrivalStation(cachedResults[item->id()]->arrivalStation());
        }
    }
    lastsearch.lastOption=departure;
    emit journeyResult(result);
}

void ParserLondonTfl::parseSearchLaterJourney(QNetworkReply *)
{

}

void ParserLondonTfl::parseSearchEarlierJourney(QNetworkReply *)
{

}

void ParserLondonTfl::parseJourneyDetails(QNetworkReply *)
{
    //should never happen
}

void ParserLondonTfl::parseJourneyOption(const QVariantMap &object, const QString &id)
{
    JourneyDetailResultList* result = new JourneyDetailResultList;

    QVariantList legs = object.value("legs").toList();

    QDateTime arrival = QDateTime::fromString(object.value("arrivalDateTime").toString(), "yyyy-MM-ddTHH:mm:ss");
    QDateTime departure = QDateTime::fromString(object.value("startDateTime").toString(), "yyyy-MM-ddTHH:mm:ss");
    result->setArrivalDateTime(arrival);
    result->setDepartureDateTime(departure);
    int minutes=departure.secsTo(arrival)/60;
    int hours=minutes/60;
    minutes=minutes%60;
    result->setDuration(QString("%1:%2").arg(hours).arg(minutes, 2, 10, QChar('0')));

    result->setId(id);

    for(int i = 0; i < legs.count(); i++)
    {
        QVariantMap leg = legs.at(i).toMap();
        JourneyDetailResultItem* resultItem = new JourneyDetailResultItem;

        QVariantMap firstStop = leg["departurePoint"].toMap();
        QVariantMap lastStop = leg["arrivalPoint"].toMap();

        resultItem->setDepartureDateTime(QDateTime::fromString(leg.value("departureTime").toString()));
        resultItem->setArrivalDateTime(QDateTime::fromString(leg.value("arrivalTime").toString()));
        resultItem->setDepartureStation(firstStop.value("commonName").toString().replace("Underground Station", "🚇"));
        resultItem->setArrivalStation(lastStop.value("commonName").toString().replace("Underground Station", "🚇"));

        QString type = leg.value("mode").toMap().value("name").toString();

        QString typeName;
        if (type != "walking")
            typeName = leg.value("mode").toMap().value("name").toString();

        //Fallback if typeName is empty
        if (typeName.isEmpty() && !type.isEmpty()) {
            typeName = type;
            typeName[0] = type.at(0).toUpper();
        }

        const QString service = leg.value("service").toString();
        if (!service.isEmpty())
            typeName.append(" ").append(service);

        if (type == "walking") {
            const QString duration = leg.value("duration").toString();
            resultItem->setTrain(tr("%1 for %2 min").arg(typeName, duration));
        } else {
            resultItem->setTrain(typeName);
        }

        /*
        if (!firstStop.value("platform").toString().isEmpty()) {
            resultItem->setDepartureInfo(tr("Pl. %1").arg(firstStop.value("platform").toString()));
        }


        if (!lastStop.value("platform").toString().isEmpty()) {
            resultItem->setArrivalInfo(tr("Pl. %1").arg(lastStop.value("platform").toString()));
        }
        */

        // get the transport options (e.g. bus or tube lines)
        QVariantList stations = leg.value("routeOptions").toList();
        QStringList routeOptionsTrains;
        QStringList routeOptionsDirections;
        QString detailedInfo;

        QVariantList::const_iterator it_routeOpt;
        for (it_routeOpt = stations.constBegin(); it_routeOpt != stations.constEnd(); ++it_routeOpt) {

            QStringList routeOptionsDirectionsCurrentTrain;
            QVariantMap routeOption = it_routeOpt->toMap();

            QString currentRouteOption = routeOption.value("name").toString().toHtmlEscaped();
            routeOptionsTrains.push_back(currentRouteOption);
            detailedInfo += currentRouteOption + " to ";

            QVariantList directions = routeOption["directions"].toList();

             QVariantList::const_iterator it_directions;

             for (it_directions = directions.constBegin(); it_directions != directions.constEnd(); ++it_directions)
             {
                 QString currentDestination = it_directions->toString().replace("Underground Station", "").toHtmlEscaped();
                 routeOptionsDirections.push_back(currentDestination);
                 routeOptionsDirectionsCurrentTrain.push_back(currentDestination);
             }

             detailedInfo += routeOptionsDirectionsCurrentTrain.join(" or ") + ".<br>";
        }

        //resultItem->setTrain(routeOptionsTrains.join(","));
        resultItem->setDirection(routeOptionsDirections.join(" or "));

        resultItem->setInfo(detailedInfo);

        //resultItem->setDirection(leg.value("destination").toString());

        QStringList attributes;
        const QVariantList attrs = leg.value("attributes").toList();
        foreach (const QVariant &attr, attrs) {
            attributes << attr.toMap().value("title").toString();
        }
        //resultItem->setInfo(attributes.join(tr(", ")));



        result->appendItem(resultItem);
    }

    result->setDepartureStation(result->getItem(0)->departureStation());
    result->setArrivalStation(result->getItem(result->itemcount() - 1)->arrivalStation());

    cachedResults.insert(id, result);
}

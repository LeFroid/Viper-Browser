#ifndef ADBLOCK_RECOMMENDED_SUBSCRIPTIONS_H
#define ADBLOCK_RECOMMENDED_SUBSCRIPTIONS_H

#include <utility>
#include <vector>

#include <QString>
#include <QUrl>

namespace adblock
{

/**
 * @class RecommendedSubscriptions
 * @brief Contains a list of recommended ad-block style filter subscriptions.
 *        The data is populated from the resource ":/adblock_recommended.json"
 */
class RecommendedSubscriptions final : public std::vector<std::pair<QString, QUrl>>
{
public:
    /// Constructs the recommended subscription container, pre-loading all subscription pairs
    RecommendedSubscriptions();

private:
    /// Loads the recommendations
    void load();
};

}

#endif // ADBLOCK_RECOMMENDED_SUBSCRIPTIONS_H

#ifndef UDHO_NET_PROTOCOL_SCGI_H
#define UDHO_NET_PROTOCOL_SCGI_H

#include <map>
#include <iostream>
#include <iterator>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <boost/utility/string_view.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <udho/utils/string_view.h>
#include <boost/asio/basic_waitable_timer.hpp>

namespace udho{
namespace net{
namespace protocols{



template <typename StreamT>
struct scgi_reader: public std::enable_shared_from_this<scgi_reader<StreamT>>{
    using handler_type     = std::function<void (std::error_code, std::size_t)>;
    using field_map_type   = std::map<std::string, boost::beast::http::field>;
    using stream_type      = StreamT;

    inline explicit scgi_reader(types::headers::request& request, stream_type& stream): _request(request), _stream(stream) {
        _field_map = {
            {"HTTP-USER-AGENT",     boost::beast::http::field::user_agent},
            {"HTTP-CONNECTION",     boost::beast::http::field::connection},
            {"HTTP-HOST",           boost::beast::http::field::host},
            {"HTTP-CACHE-CONTROL",  boost::beast::http::field::cache_control},
            {"DOCUMENT-URI",        boost::beast::http::field::uri},
            {"HTTP-ACCEPT",         boost::beast::http::field::accept}
        };
    }

    template <typename Handler>
    void start(Handler&& handler){
        std::cout << "started reading" << std::endl;
        boost::asio::async_read_until(
            _stream, _buffer, "\0,",
            std::bind(&scgi_reader::finished, std::enable_shared_from_this<scgi_reader<StreamT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
        _handler = std::move(handler);
    }
    private:
        void finished(std::error_code ec, std::size_t bytes_transferred){
            if(!ec){
                parse();
            }
            std::cout << "finished" << std::endl;
            _handler(ec, bytes_transferred);
        }
        void parse(){
            std::istream stream(&_buffer);
            std::string word, last_key;
            bool is_key = true;
            boost::beast::http::field field;
            std::getline(stream, word, ':');
            while (std::getline(stream, word, '\0')) {
                if(is_key){
                    last_key = (boost::beast::http::string_to_field(word) == boost::beast::http::field::unknown) ? boost::replace_all_copy(word, "_", "-") : word;
                    field = boost::beast::http::string_to_field(last_key);
                }else{
                    bool known_field = (field != boost::beast::http::field::unknown);
                    if(!known_field){
                        std::string key_upper = boost::algorithm::to_upper_copy(last_key);
                        if(key_upper == "REQUEST-URI"){
                            _request.target(word);
                        }else if(key_upper == "REQUEST-METHOD"){
                            _request.method_string(word);
                        }else{
                            auto it = _field_map.find(key_upper);
                            if(it != _field_map.end()){
                                _request.insert(it->second, word);
                            }else{
                                std::cout << "unknown field: " << last_key << " value: " << word << std::endl;
                            }
                        }
                    }else{
                        _request.insert(field, word);
                    }
                }
                is_key = !is_key;
            }
        }
    private:
        udho::net::types::headers::request& _request;
        boost::asio::streambuf              _buffer;
        handler_type                        _handler;
        stream_type&                        _stream;
        field_map_type                      _field_map;
};

namespace detail{

struct fields{
    static const std::unordered_map<udho::utils::string_view, boost::beast::http::field>& table() {
        const static std::unordered_map<udho::utils::string_view, boost::beast::http::field> fields_map{
          {"<UNKNOWN_FIELD>",boost::beast::http::field::unknown},
          {"A_IM",boost::beast::http::field::a_im},
          {"ACCEPT",boost::beast::http::field::accept},
          {"ACCEPT_ADDITIONS",boost::beast::http::field::accept_additions},
          {"ACCEPT_CHARSET",boost::beast::http::field::accept_charset},
          {"ACCEPT_DATETIME",boost::beast::http::field::accept_datetime},
          {"ACCEPT_ENCODING",boost::beast::http::field::accept_encoding},
          {"ACCEPT_FEATURES",boost::beast::http::field::accept_features},
          {"ACCEPT_LANGUAGE",boost::beast::http::field::accept_language},
          {"ACCEPT_PATCH",boost::beast::http::field::accept_patch},
          {"ACCEPT_POST",boost::beast::http::field::accept_post},
          {"ACCEPT_RANGES",boost::beast::http::field::accept_ranges},
          {"ACCESS_CONTROL",boost::beast::http::field::access_control},
          {"ACCESS_CONTROL_ALLOW_CREDENTIALS",boost::beast::http::field::access_control_allow_credentials},
          {"ACCESS_CONTROL_ALLOW_HEADERS",boost::beast::http::field::access_control_allow_headers},
          {"ACCESS_CONTROL_ALLOW_METHODS",boost::beast::http::field::access_control_allow_methods},
          {"ACCESS_CONTROL_ALLOW_ORIGIN",boost::beast::http::field::access_control_allow_origin},
          {"ACCESS_CONTROL_EXPOSE_HEADERS",boost::beast::http::field::access_control_expose_headers},
          {"ACCESS_CONTROL_MAX_AGE",boost::beast::http::field::access_control_max_age},
          {"ACCESS_CONTROL_REQUEST_HEADERS",boost::beast::http::field::access_control_request_headers},
          {"ACCESS_CONTROL_REQUEST_METHOD",boost::beast::http::field::access_control_request_method},
          {"AGE",boost::beast::http::field::age},
          {"ALLOW",boost::beast::http::field::allow},
          {"ALPN",boost::beast::http::field::alpn},
          {"ALSO_CONTROL",boost::beast::http::field::also_control},
          {"ALT_SVC",boost::beast::http::field::alt_svc},
          {"ALT_USED",boost::beast::http::field::alt_used},
          {"ALTERNATE_RECIPIENT",boost::beast::http::field::alternate_recipient},
          {"ALTERNATES",boost::beast::http::field::alternates},
          {"APPARENTLY_TO",boost::beast::http::field::apparently_to},
          {"APPLY_TO_REDIRECT_REF",boost::beast::http::field::apply_to_redirect_ref},
          {"APPROVED",boost::beast::http::field::approved},
          {"ARCHIVE",boost::beast::http::field::archive},
          {"ARCHIVED_AT",boost::beast::http::field::archived_at},
          {"ARTICLE_NAMES",boost::beast::http::field::article_names},
          {"ARTICLE_UPDATES",boost::beast::http::field::article_updates},
          {"AUTHENTICATION_CONTROL",boost::beast::http::field::authentication_control},
          {"AUTHENTICATION_INFO",boost::beast::http::field::authentication_info},
          {"AUTHENTICATION_RESULTS",boost::beast::http::field::authentication_results},
          {"AUTHORIZATION",boost::beast::http::field::authorization},
          {"AUTO_SUBMITTED",boost::beast::http::field::auto_submitted},
          {"AUTOFORWARDED",boost::beast::http::field::autoforwarded},
          {"AUTOSUBMITTED",boost::beast::http::field::autosubmitted},
          {"BASE",boost::beast::http::field::base},
          {"BCC",boost::beast::http::field::bcc},
          {"BODY",boost::beast::http::field::body},
          {"C_EXT",boost::beast::http::field::c_ext},
          {"C_MAN",boost::beast::http::field::c_man},
          {"C_OPT",boost::beast::http::field::c_opt},
          {"C_PEP",boost::beast::http::field::c_pep},
          {"C_PEP_INFO",boost::beast::http::field::c_pep_info},
          {"CACHE_CONTROL",boost::beast::http::field::cache_control},
          {"CALDAV_TIMEZONES",boost::beast::http::field::caldav_timezones},
          {"CANCEL_KEY",boost::beast::http::field::cancel_key},
          {"CANCEL_LOCK",boost::beast::http::field::cancel_lock},
          {"CC",boost::beast::http::field::cc},
          {"CLOSE",boost::beast::http::field::close},
          {"COMMENTS",boost::beast::http::field::comments},
          {"COMPLIANCE",boost::beast::http::field::compliance},
          {"CONNECTION",boost::beast::http::field::connection},
          {"CONTENT_ALTERNATIVE",boost::beast::http::field::content_alternative},
          {"CONTENT_BASE",boost::beast::http::field::content_base},
          {"CONTENT_DESCRIPTION",boost::beast::http::field::content_description},
          {"CONTENT_DISPOSITION",boost::beast::http::field::content_disposition},
          {"CONTENT_DURATION",boost::beast::http::field::content_duration},
          {"CONTENT_ENCODING",boost::beast::http::field::content_encoding},
          {"CONTENT_FEATURES",boost::beast::http::field::content_features},
          {"CONTENT_ID",boost::beast::http::field::content_id},
          {"CONTENT_IDENTIFIER",boost::beast::http::field::content_identifier},
          {"CONTENT_LANGUAGE",boost::beast::http::field::content_language},
          {"CONTENT_LENGTH",boost::beast::http::field::content_length},
          {"CONTENT_LOCATION",boost::beast::http::field::content_location},
          {"CONTENT_MD5",boost::beast::http::field::content_md5},
          {"CONTENT_RANGE",boost::beast::http::field::content_range},
          {"CONTENT_RETURN",boost::beast::http::field::content_return},
          {"CONTENT_SCRIPT_TYPE",boost::beast::http::field::content_script_type},
          {"CONTENT_STYLE_TYPE",boost::beast::http::field::content_style_type},
          {"CONTENT_TRANSFER_ENCODING",boost::beast::http::field::content_transfer_encoding},
          {"CONTENT_TYPE",boost::beast::http::field::content_type},
          {"CONTENT_VERSION",boost::beast::http::field::content_version},
          {"CONTROL",boost::beast::http::field::control},
          {"CONVERSION",boost::beast::http::field::conversion},
          {"CONVERSION_WITH_LOSS",boost::beast::http::field::conversion_with_loss},
          {"COOKIE",boost::beast::http::field::cookie},
          {"COOKIE2",boost::beast::http::field::cookie2},
          {"COST",boost::beast::http::field::cost},
          {"DASL",boost::beast::http::field::dasl},
          {"DATE",boost::beast::http::field::date},
          {"DATE_RECEIVED",boost::beast::http::field::date_received},
          {"DAV",boost::beast::http::field::dav},
          {"DEFAULT_STYLE",boost::beast::http::field::default_style},
          {"DEFERRED_DELIVERY",boost::beast::http::field::deferred_delivery},
          {"DELIVERY_DATE",boost::beast::http::field::delivery_date},
          {"DELTA_BASE",boost::beast::http::field::delta_base},
          {"DEPTH",boost::beast::http::field::depth},
          {"DERIVED_FROM",boost::beast::http::field::derived_from},
          {"DESTINATION",boost::beast::http::field::destination},
          {"DIFFERENTIAL_ID",boost::beast::http::field::differential_id},
          {"DIGEST",boost::beast::http::field::digest},
          {"DISCARDED_X400_IPMS_EXTENSIONS",boost::beast::http::field::discarded_x400_ipms_extensions},
          {"DISCARDED_X400_MTS_EXTENSIONS",boost::beast::http::field::discarded_x400_mts_extensions},
          {"DISCLOSE_RECIPIENTS",boost::beast::http::field::disclose_recipients},
          {"DISPOSITION_NOTIFICATION_OPTIONS",boost::beast::http::field::disposition_notification_options},
          {"DISPOSITION_NOTIFICATION_TO",boost::beast::http::field::disposition_notification_to},
          {"DISTRIBUTION",boost::beast::http::field::distribution},
          {"DKIM_SIGNATURE",boost::beast::http::field::dkim_signature},
          {"DL_EXPANSION_HISTORY",boost::beast::http::field::dl_expansion_history},
          {"DOWNGRADED_BCC",boost::beast::http::field::downgraded_bcc},
          {"DOWNGRADED_CC",boost::beast::http::field::downgraded_cc},
          {"DOWNGRADED_DISPOSITION_NOTIFICATION_TO",boost::beast::http::field::downgraded_disposition_notification_to},
          {"DOWNGRADED_FINAL_RECIPIENT",boost::beast::http::field::downgraded_final_recipient},
          {"DOWNGRADED_FROM",boost::beast::http::field::downgraded_from},
          {"DOWNGRADED_IN_REPLY_TO",boost::beast::http::field::downgraded_in_reply_to},
          {"DOWNGRADED_MAIL_FROM",boost::beast::http::field::downgraded_mail_from},
          {"DOWNGRADED_MESSAGE_ID",boost::beast::http::field::downgraded_message_id},
          {"DOWNGRADED_ORIGINAL_RECIPIENT",boost::beast::http::field::downgraded_original_recipient},
          {"DOWNGRADED_RCPT_TO",boost::beast::http::field::downgraded_rcpt_to},
          {"DOWNGRADED_REFERENCES",boost::beast::http::field::downgraded_references},
          {"DOWNGRADED_REPLY_TO",boost::beast::http::field::downgraded_reply_to},
          {"DOWNGRADED_RESENT_BCC",boost::beast::http::field::downgraded_resent_bcc},
          {"DOWNGRADED_RESENT_CC",boost::beast::http::field::downgraded_resent_cc},
          {"DOWNGRADED_RESENT_FROM",boost::beast::http::field::downgraded_resent_from},
          {"DOWNGRADED_RESENT_REPLY_TO",boost::beast::http::field::downgraded_resent_reply_to},
          {"DOWNGRADED_RESENT_SENDER",boost::beast::http::field::downgraded_resent_sender},
          {"DOWNGRADED_RESENT_TO",boost::beast::http::field::downgraded_resent_to},
          {"DOWNGRADED_RETURN_PATH",boost::beast::http::field::downgraded_return_path},
          {"DOWNGRADED_SENDER",boost::beast::http::field::downgraded_sender},
          {"DOWNGRADED_TO",boost::beast::http::field::downgraded_to},
          {"EDIINT_FEATURES",boost::beast::http::field::ediint_features},
          {"EESST_VERSION",boost::beast::http::field::eesst_version},
          {"ENCODING",boost::beast::http::field::encoding},
          {"ENCRYPTED",boost::beast::http::field::encrypted},
          {"ERRORS_TO",boost::beast::http::field::errors_to},
          {"ETAG",boost::beast::http::field::etag},
          {"EXPECT",boost::beast::http::field::expect},
          {"EXPIRES",boost::beast::http::field::expires},
          {"EXPIRY_DATE",boost::beast::http::field::expiry_date},
          {"EXT",boost::beast::http::field::ext},
          {"FOLLOWUP_TO",boost::beast::http::field::followup_to},
          {"FORWARDED",boost::beast::http::field::forwarded},
          {"FROM",boost::beast::http::field::from},
          {"GENERATE_DELIVERY_REPORT",boost::beast::http::field::generate_delivery_report},
          {"GETPROFILE",boost::beast::http::field::getprofile},
          {"HOBAREG",boost::beast::http::field::hobareg},
          {"HOST",boost::beast::http::field::host},
          {"HTTP2_SETTINGS",boost::beast::http::field::http2_settings},
          {"IF",boost::beast::http::field::if_},
          {"IF_MATCH",boost::beast::http::field::if_match},
          {"IF_MODIFIED_SINCE",boost::beast::http::field::if_modified_since},
          {"IF_NONE_MATCH",boost::beast::http::field::if_none_match},
          {"IF_RANGE",boost::beast::http::field::if_range},
          {"IF_SCHEDULE_TAG_MATCH",boost::beast::http::field::if_schedule_tag_match},
          {"IF_UNMODIFIED_SINCE",boost::beast::http::field::if_unmodified_since},
          {"IM",boost::beast::http::field::im},
          {"IMPORTANCE",boost::beast::http::field::importance},
          {"IN_REPLY_TO",boost::beast::http::field::in_reply_to},
          {"INCOMPLETE_COPY",boost::beast::http::field::incomplete_copy},
          {"INJECTION_DATE",boost::beast::http::field::injection_date},
          {"INJECTION_INFO",boost::beast::http::field::injection_info},
          {"JABBER_ID",boost::beast::http::field::jabber_id},
          {"KEEP_ALIVE",boost::beast::http::field::keep_alive},
          {"KEYWORDS",boost::beast::http::field::keywords},
          {"LABEL",boost::beast::http::field::label},
          {"LANGUAGE",boost::beast::http::field::language},
          {"LAST_MODIFIED",boost::beast::http::field::last_modified},
          {"LATEST_DELIVERY_TIME",boost::beast::http::field::latest_delivery_time},
          {"LINES",boost::beast::http::field::lines},
          {"LINK",boost::beast::http::field::link},
          {"LIST_ARCHIVE",boost::beast::http::field::list_archive},
          {"LIST_HELP",boost::beast::http::field::list_help},
          {"LIST_ID",boost::beast::http::field::list_id},
          {"LIST_OWNER",boost::beast::http::field::list_owner},
          {"LIST_POST",boost::beast::http::field::list_post},
          {"LIST_SUBSCRIBE",boost::beast::http::field::list_subscribe},
          {"LIST_UNSUBSCRIBE",boost::beast::http::field::list_unsubscribe},
          {"LIST_UNSUBSCRIBE_POST",boost::beast::http::field::list_unsubscribe_post},
          {"LOCATION",boost::beast::http::field::location},
          {"LOCK_TOKEN",boost::beast::http::field::lock_token},
          {"MAN",boost::beast::http::field::man},
          {"MAX_FORWARDS",boost::beast::http::field::max_forwards},
          {"MEMENTO_DATETIME",boost::beast::http::field::memento_datetime},
          {"MESSAGE_CONTEXT",boost::beast::http::field::message_context},
          {"MESSAGE_ID",boost::beast::http::field::message_id},
          {"MESSAGE_TYPE",boost::beast::http::field::message_type},
          {"METER",boost::beast::http::field::meter},
          {"METHOD_CHECK",boost::beast::http::field::method_check},
          {"METHOD_CHECK_EXPIRES",boost::beast::http::field::method_check_expires},
          {"MIME_VERSION",boost::beast::http::field::mime_version},
          {"MMHS_ACP127_MESSAGE_IDENTIFIER",boost::beast::http::field::mmhs_acp127_message_identifier},
          {"MMHS_AUTHORIZING_USERS",boost::beast::http::field::mmhs_authorizing_users},
          {"MMHS_CODRESS_MESSAGE_INDICATOR",boost::beast::http::field::mmhs_codress_message_indicator},
          {"MMHS_COPY_PRECEDENCE",boost::beast::http::field::mmhs_copy_precedence},
          {"MMHS_EXEMPTED_ADDRESS",boost::beast::http::field::mmhs_exempted_address},
          {"MMHS_EXTENDED_AUTHORISATION_INFO",boost::beast::http::field::mmhs_extended_authorisation_info},
          {"MMHS_HANDLING_INSTRUCTIONS",boost::beast::http::field::mmhs_handling_instructions},
          {"MMHS_MESSAGE_INSTRUCTIONS",boost::beast::http::field::mmhs_message_instructions},
          {"MMHS_MESSAGE_TYPE",boost::beast::http::field::mmhs_message_type},
          {"MMHS_ORIGINATOR_PLAD",boost::beast::http::field::mmhs_originator_plad},
          {"MMHS_ORIGINATOR_REFERENCE",boost::beast::http::field::mmhs_originator_reference},
          {"MMHS_OTHER_RECIPIENTS_INDICATOR_CC",boost::beast::http::field::mmhs_other_recipients_indicator_cc},
          {"MMHS_OTHER_RECIPIENTS_INDICATOR_TO",boost::beast::http::field::mmhs_other_recipients_indicator_to},
          {"MMHS_PRIMARY_PRECEDENCE",boost::beast::http::field::mmhs_primary_precedence},
          {"MMHS_SUBJECT_INDICATOR_CODES",boost::beast::http::field::mmhs_subject_indicator_codes},
          {"MT_PRIORITY",boost::beast::http::field::mt_priority},
          {"NEGOTIATE",boost::beast::http::field::negotiate},
          {"NEWSGROUPS",boost::beast::http::field::newsgroups},
          {"NNTP_POSTING_DATE",boost::beast::http::field::nntp_posting_date},
          {"NNTP_POSTING_HOST",boost::beast::http::field::nntp_posting_host},
          {"NON_COMPLIANCE",boost::beast::http::field::non_compliance},
          {"OBSOLETES",boost::beast::http::field::obsoletes},
          {"OPT",boost::beast::http::field::opt},
          {"OPTIONAL",boost::beast::http::field::optional},
          {"OPTIONAL_WWW_AUTHENTICATE",boost::beast::http::field::optional_www_authenticate},
          {"ORDERING_TYPE",boost::beast::http::field::ordering_type},
          {"ORGANIZATION",boost::beast::http::field::organization},
          {"ORIGIN",boost::beast::http::field::origin},
          {"ORIGINAL_ENCODED_INFORMATION_TYPES",boost::beast::http::field::original_encoded_information_types},
          {"ORIGINAL_FROM",boost::beast::http::field::original_from},
          {"ORIGINAL_MESSAGE_ID",boost::beast::http::field::original_message_id},
          {"ORIGINAL_RECIPIENT",boost::beast::http::field::original_recipient},
          {"ORIGINAL_SENDER",boost::beast::http::field::original_sender},
          {"ORIGINAL_SUBJECT",boost::beast::http::field::original_subject},
          {"ORIGINATOR_RETURN_ADDRESS",boost::beast::http::field::originator_return_address},
          {"OVERWRITE",boost::beast::http::field::overwrite},
          {"P3P",boost::beast::http::field::p3p},
          {"PATH",boost::beast::http::field::path},
          {"PEP",boost::beast::http::field::pep},
          {"PEP_INFO",boost::beast::http::field::pep_info},
          {"PICS_LABEL",boost::beast::http::field::pics_label},
          {"POSITION",boost::beast::http::field::position},
          {"POSTING_VERSION",boost::beast::http::field::posting_version},
          {"PRAGMA",boost::beast::http::field::pragma},
          {"PREFER",boost::beast::http::field::prefer},
          {"PREFERENCE_APPLIED",boost::beast::http::field::preference_applied},
          {"PREVENT_NONDELIVERY_REPORT",boost::beast::http::field::prevent_nondelivery_report},
          {"PRIORITY",boost::beast::http::field::priority},
          {"PRIVICON",boost::beast::http::field::privicon},
          {"PROFILEOBJECT",boost::beast::http::field::profileobject},
          {"PROTOCOL",boost::beast::http::field::protocol},
          {"PROTOCOL_INFO",boost::beast::http::field::protocol_info},
          {"PROTOCOL_QUERY",boost::beast::http::field::protocol_query},
          {"PROTOCOL_REQUEST",boost::beast::http::field::protocol_request},
          {"PROXY_AUTHENTICATE",boost::beast::http::field::proxy_authenticate},
          {"PROXY_AUTHENTICATION_INFO",boost::beast::http::field::proxy_authentication_info},
          {"PROXY_AUTHORIZATION",boost::beast::http::field::proxy_authorization},
          {"PROXY_CONNECTION",boost::beast::http::field::proxy_connection},
          {"PROXY_FEATURES",boost::beast::http::field::proxy_features},
          {"PROXY_INSTRUCTION",boost::beast::http::field::proxy_instruction},
          {"PUBLIC",boost::beast::http::field::public_},
          {"PUBLIC_KEY_PINS",boost::beast::http::field::public_key_pins},
          {"PUBLIC_KEY_PINS_REPORT_ONLY",boost::beast::http::field::public_key_pins_report_only},
          {"RANGE",boost::beast::http::field::range},
          {"RECEIVED",boost::beast::http::field::received},
          {"RECEIVED_SPF",boost::beast::http::field::received_spf},
          {"REDIRECT_REF",boost::beast::http::field::redirect_ref},
          {"REFERENCES",boost::beast::http::field::references},
          {"REFERER",boost::beast::http::field::referer},
          {"REFERER_ROOT",boost::beast::http::field::referer_root},
          {"RELAY_VERSION",boost::beast::http::field::relay_version},
          {"REPLY_BY",boost::beast::http::field::reply_by},
          {"REPLY_TO",boost::beast::http::field::reply_to},
          {"REQUIRE_RECIPIENT_VALID_SINCE",boost::beast::http::field::require_recipient_valid_since},
          {"RESENT_BCC",boost::beast::http::field::resent_bcc},
          {"RESENT_CC",boost::beast::http::field::resent_cc},
          {"RESENT_DATE",boost::beast::http::field::resent_date},
          {"RESENT_FROM",boost::beast::http::field::resent_from},
          {"RESENT_MESSAGE_ID",boost::beast::http::field::resent_message_id},
          {"RESENT_REPLY_TO",boost::beast::http::field::resent_reply_to},
          {"RESENT_SENDER",boost::beast::http::field::resent_sender},
          {"RESENT_TO",boost::beast::http::field::resent_to},
          {"RESOLUTION_HINT",boost::beast::http::field::resolution_hint},
          {"RESOLVER_LOCATION",boost::beast::http::field::resolver_location},
          {"RETRY_AFTER",boost::beast::http::field::retry_after},
          {"RETURN_PATH",boost::beast::http::field::return_path},
          {"SAFE",boost::beast::http::field::safe},
          {"SCHEDULE_REPLY",boost::beast::http::field::schedule_reply},
          {"SCHEDULE_TAG",boost::beast::http::field::schedule_tag},
          {"SEC_FETCH_DEST",boost::beast::http::field::sec_fetch_dest},
          {"SEC_FETCH_MODE",boost::beast::http::field::sec_fetch_mode},
          {"SEC_FETCH_SITE",boost::beast::http::field::sec_fetch_site},
          {"SEC_FETCH_USER",boost::beast::http::field::sec_fetch_user},
          {"SEC_WEBSOCKET_ACCEPT",boost::beast::http::field::sec_websocket_accept},
          {"SEC_WEBSOCKET_EXTENSIONS",boost::beast::http::field::sec_websocket_extensions},
          {"SEC_WEBSOCKET_KEY",boost::beast::http::field::sec_websocket_key},
          {"SEC_WEBSOCKET_PROTOCOL",boost::beast::http::field::sec_websocket_protocol},
          {"SEC_WEBSOCKET_VERSION",boost::beast::http::field::sec_websocket_version},
          {"SECURITY_SCHEME",boost::beast::http::field::security_scheme},
          {"SEE_ALSO",boost::beast::http::field::see_also},
          {"SENDER",boost::beast::http::field::sender},
          {"SENSITIVITY",boost::beast::http::field::sensitivity},
          {"SERVER",boost::beast::http::field::server},
          {"SET_COOKIE",boost::beast::http::field::set_cookie},
          {"SET_COOKIE2",boost::beast::http::field::set_cookie2},
          {"SETPROFILE",boost::beast::http::field::setprofile},
          {"SIO_LABEL",boost::beast::http::field::sio_label},
          {"SIO_LABEL_HISTORY",boost::beast::http::field::sio_label_history},
          {"SLUG",boost::beast::http::field::slug},
          {"SOAPACTION",boost::beast::http::field::soapaction},
          {"SOLICITATION",boost::beast::http::field::solicitation},
          {"STATUS_URI",boost::beast::http::field::status_uri},
          {"STRICT_TRANSPORT_SECURITY",boost::beast::http::field::strict_transport_security},
          {"SUBJECT",boost::beast::http::field::subject},
          {"SUBOK",boost::beast::http::field::subok},
          {"SUBST",boost::beast::http::field::subst},
          {"SUMMARY",boost::beast::http::field::summary},
          {"SUPERSEDES",boost::beast::http::field::supersedes},
          {"SURROGATE_CAPABILITY",boost::beast::http::field::surrogate_capability},
          {"SURROGATE_CONTROL",boost::beast::http::field::surrogate_control},
          {"TCN",boost::beast::http::field::tcn},
          {"TE",boost::beast::http::field::te},
          {"TIMEOUT",boost::beast::http::field::timeout},
          {"TITLE",boost::beast::http::field::title},
          {"TO",boost::beast::http::field::to},
          {"TOPIC",boost::beast::http::field::topic},
          {"TRAILER",boost::beast::http::field::trailer},
          {"TRANSFER_ENCODING",boost::beast::http::field::transfer_encoding},
          {"TTL",boost::beast::http::field::ttl},
          {"UA_COLOR",boost::beast::http::field::ua_color},
          {"UA_MEDIA",boost::beast::http::field::ua_media},
          {"UA_PIXELS",boost::beast::http::field::ua_pixels},
          {"UA_RESOLUTION",boost::beast::http::field::ua_resolution},
          {"UA_WINDOWPIXELS",boost::beast::http::field::ua_windowpixels},
          {"UPGRADE",boost::beast::http::field::upgrade},
          {"URGENCY",boost::beast::http::field::urgency},
          {"URI",boost::beast::http::field::uri},
          {"USER_AGENT",boost::beast::http::field::user_agent},
          {"VARIANT_VARY",boost::beast::http::field::variant_vary},
          {"VARY",boost::beast::http::field::vary},
          {"VBR_INFO",boost::beast::http::field::vbr_info},
          {"VERSION",boost::beast::http::field::version},
          {"VIA",boost::beast::http::field::via},
          {"WANT_DIGEST",boost::beast::http::field::want_digest},
          {"WARNING",boost::beast::http::field::warning},
          {"WWW_AUTHENTICATE",boost::beast::http::field::www_authenticate},
          {"X_ARCHIVED_AT",boost::beast::http::field::x_archived_at},
          {"X_DEVICE_ACCEPT",boost::beast::http::field::x_device_accept},
          {"X_DEVICE_ACCEPT_CHARSET",boost::beast::http::field::x_device_accept_charset},
          {"X_DEVICE_ACCEPT_ENCODING",boost::beast::http::field::x_device_accept_encoding},
          {"X_DEVICE_ACCEPT_LANGUAGE",boost::beast::http::field::x_device_accept_language},
          {"X_DEVICE_USER_AGENT",boost::beast::http::field::x_device_user_agent},
          {"X_FRAME_OPTIONS",boost::beast::http::field::x_frame_options},
          {"X_MITTENTE",boost::beast::http::field::x_mittente},
          {"X_PGP_SIG",boost::beast::http::field::x_pgp_sig},
          {"X_RICEVUTA",boost::beast::http::field::x_ricevuta},
          {"X_RIFERIMENTO_MESSAGE_ID",boost::beast::http::field::x_riferimento_message_id},
          {"X_TIPORICEVUTA",boost::beast::http::field::x_tiporicevuta},
          {"X_TRASPORTO",boost::beast::http::field::x_trasporto},
          {"X_VERIFICASICUREZZA",boost::beast::http::field::x_verificasicurezza},
          {"X400_CONTENT_IDENTIFIER",boost::beast::http::field::x400_content_identifier},
          {"X400_CONTENT_RETURN",boost::beast::http::field::x400_content_return},
          {"X400_CONTENT_TYPE",boost::beast::http::field::x400_content_type},
          {"X400_MTS_IDENTIFIER",boost::beast::http::field::x400_mts_identifier},
          {"X400_ORIGINATOR",boost::beast::http::field::x400_originator},
          {"X400_RECEIVED",boost::beast::http::field::x400_received},
          {"X400_RECIPIENTS",boost::beast::http::field::x400_recipients},
          {"X400_TRACE",boost::beast::http::field::x400_trace},
          {"XREF",boost::beast::http::field::xref}
        };
        return fields_map;
    }
};

}

template <typename StreamT>
struct scgi_reader2: public std::enable_shared_from_this<scgi_reader2<StreamT>>{
    using stream_type      = StreamT;
    using timer_type       = boost::asio::steady_timer;
    using time_unit        = std::chrono::seconds;

    inline explicit scgi_reader2(stream_type& stream, std::size_t wait_til = 2): _stream(stream), _timer(stream.get_executor()), _wait_til(wait_til) { }

    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds) {
        auto self = this->shared_from_this();
        boost::asio::async_read_until(
            _stream, _buffer, ":",
            [this, self, handler = std::move(handler)] (std::error_code ec, std::size_t bytes_transferred) mutable {
                _timer.cancel();
                if (ec) {
                    handler(udho::net::types::headers::request{}, ec, bytes_transferred);
                    return;
                }
                self->read_length(std::move(handler), bytes_transferred);
            }
        );
        _timer.expires_after(_wait_til);
        _timer.async_wait([self](const std::error_code& error){
            if (!error) {
                self->_stream.close();
            }
        });
    }
private:
    template <typename Handler>
    void read_length(Handler&& handler, std::size_t bytes_transferred) {
        std::istream stream(&_buffer);
        std::string length_str;
        std::getline(stream, length_str, ':');

        if (length_str.empty() || !std::all_of(length_str.begin(), length_str.end(), ::isdigit) || length_str.size() > 5) {
            _timer.cancel();
            handler(udho::net::types::headers::request{}, std::make_error_code(std::errc::invalid_argument), 0);
            return;
        }

        std::size_t len = 0;
        try {
            len = static_cast<std::size_t>(std::stoull(length_str));
        } catch (...) {
            _timer.cancel();
            handler(udho::net::types::headers::request{}, std::make_error_code(std::errc::invalid_argument), 0);
            return;
        }

        auto self = this->shared_from_this();
        try {
            std::size_t already = _buffer.size();     // bytes remaining after getline consumed "len:"
            if (already >= len) {
                self->read_comma(std::forward<Handler>(handler), len);
            } else {
                std::size_t remaining = len - already;
                boost::asio::async_read(
                    _stream, _buffer, boost::asio::transfer_exactly(remaining),
                    [self, len, handler = std::forward<Handler>(handler)] (std::error_code ec, std::size_t bytes_transferred) mutable {
                        self->_timer.cancel();
                        if (ec) {
                            handler(udho::net::types::headers::request{}, ec, bytes_transferred);
                            return;
                        }
                        self->read_comma(std::move(handler), len);
                    }
                );

                _timer.expires_after(_wait_til);
                _timer.async_wait([self](const std::error_code& error){
                    if (!error) {
                        self->_stream.close();
                    }
                });
            }
        } catch (const std::exception& e) {
            _timer.cancel();
            handler(udho::net::types::headers::request{}, std::make_error_code(std::errc::invalid_argument), bytes_transferred);
        }
    }

    template <typename Handler>
    void read_comma(Handler&& handler, std::size_t length) {
        auto self = this->shared_from_this();
        std::size_t already = _buffer.size();

        if (already > length) {
            _data.assign(boost::asio::buffers_begin(_buffer.data()), boost::asio::buffers_end(_buffer.data()));
            char coma = _data.back();
            _data.pop_back();

            if (coma != ',') {
                handler(udho::net::types::headers::request{}, std::make_error_code(std::errc::invalid_argument), length+1);
                return;
            }

            self->parse_scgi();
            handler(std::move(_request), std::error_code{}, length+1);
        } else {
            boost::asio::async_read(
                _stream, _buffer, boost::asio::transfer_exactly(1),
                [this, self, handler = std::move(handler), length] (std::error_code ec, std::size_t bytes_transferred) mutable {
                    _timer.cancel();
                    if (ec) {
                        handler(udho::net::types::headers::request{}, ec, bytes_transferred);
                        return;
                    }

                    _data.assign(boost::asio::buffers_begin(_buffer.data()), boost::asio::buffers_end(_buffer.data()));
                    char coma = _data.back();
                    _data.pop_back();

                    if (coma != ',') {
                        handler(udho::net::types::headers::request{}, std::make_error_code(std::errc::invalid_argument), bytes_transferred);
                        return;
                    }

                    self->parse_scgi();
                    handler(std::move(_request), std::error_code{}, length+1);
                }
            );

            _timer.expires_after(_wait_til);
            _timer.async_wait([self](const std::error_code& error){
                if (!error) {
                    self->_stream.close();
                }
            });
        }
    }

    void parse_scgi() {
        udho::utils::string_view view{_data.c_str(), _data.size()};
        udho::utils::string_view::size_type last = 0;
        udho::utils::string_view::size_type pos  = view.find('\0', last);
        udho::utils::string_view last_key;
        std::size_t count = 0;
        while(pos != udho::utils::string_view::npos) {
            auto size = pos - last;
            udho::utils::string_view substr = view.substr(last, size);

            if(count++ % 2 == 0) {
                last_key = substr;
            } else {
                udho::utils::string_view value{substr};
                process_key(last_key, value);
            }

            last = pos+1;
            pos  = view.find('\0', last);
        }
    }

    void process_key(const udho::utils::string_view& key, const udho::utils::string_view& value) {
        if (key == "REQUEST_METHOD") {
            _request.method(boost::beast::http::string_to_verb(value));
        } else if (key == "REQUEST_URI" || key == "DOCUMENT_URI") {
            _request.target(value);
        } else if (key == "SERVER_PROTOCOL") {
            if (value == "HTTP/1.0") {
                _request.version(10);
            } else if (value == "HTTP/1.1") {
                _request.version(11);
            }
        } else if (key == "CONTENT_LENGTH") {
            _request.set(boost::beast::http::field::content_length, value);
        } else if (key == "CONTENT_TYPE") {
            _request.set(boost::beast::http::field::content_type, value);
        } else {
            udho::utils::string_view header_prefix{"HTTP_"};
            auto pos = key.find(header_prefix);
            if(pos != udho::utils::string_view::npos) {
                udho::utils::string_view header = key.substr(header_prefix.size(), key.size()-header_prefix.size());
                auto it = detail::fields::table().find(header);
                if (it != detail::fields::table().cend()) {
                    boost::beast::http::field field = it->second;
                    _request.set(field, value);
                } else {
                    std::string key{header};
                    std::replace(key.begin(), key.end(), '_', '-');
                    _request.insert(key, value);
                }
            } else {
                // What do to?
            }
        }
    }
private:
    udho::net::types::headers::request  _request;
    boost::asio::streambuf              _buffer;
    stream_type&                        _stream;
    std::string                         _data;
    timer_type                          _timer;
    time_unit                           _wait_til;
};

}
}
}


#endif // UDHO_NET_PROTOCOL_SCGI_H


#include <iostream>

#include <restinio/all.hpp>

#include <json_dto/pub.hpp>

struct place_t
{
	place_t() = default;

	place_t(
		std::string placename,
		double lat,
		double lot )
		:	m_placename{ std::move( placename ) }
		,	m_lat{ std::move( lat ) }
		,	m_lot{ std::move( lot ) }
	{}

	template < typename JSON_IO >
	void
	json_io( JSON_IO & io )
	{
		io
			& json_dto::mandatory( "Placename", m_placename )
			& json_dto::mandatory( "Latitude", m_lat )
			& json_dto::mandatory( "Longitude", m_lot);
	}

	std::string m_placename;
	double m_lat;
	double m_lot;
};

struct weatherStation_t
{
	weatherStation_t() = default;

	weatherStation_t(
		int ID,
		std::string date,
		std::string time,
		place_t place,
		float temperature,
		int humidity )
		:	m_ID{ std::move( ID ) }
		,	m_date{ std::move( date ) }
		,	m_time{ std::move( time ) }
		,	m_place{ std::move( place )}
		,	m_temperature{ std::move( temperature ) }
		,	m_humidity{ std::move( humidity ) }
	{}

	template < typename JSON_IO >
	void
	json_io( JSON_IO & io )
	{
		io
			& json_dto::mandatory( "ID", m_ID )
			& json_dto::mandatory( "Date", m_date )
			& json_dto::mandatory( "Time", m_time )
			& json_dto::mandatory( "Place", m_place )
			& json_dto::mandatory( "Temperature", m_temperature )
			&json_dto::mandatory( "Humidity", m_humidity);
	}

	int m_ID;
	std::string m_date;
	std::string m_time;
	place_t m_place;
	float m_temperature;
	int m_humidity;

	
};

using weatherStation_t = std::vector< book_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class books_handler_t
{
public :
	explicit books_handler_t( weatherStation_t & books )
		:	m_books( books )
	{}

	books_handler_t( const books_handler_t & ) = delete;
	books_handler_t( books_handler_t && ) = delete;

	auto on_books_list(
		const restinio::request_handle_t& req, rr::route_params_t ) const
	{
		auto resp = init_resp( req->create_response() );

		resp.set_body(
			"Book collection (book count: " +
				std::to_string( m_books.size() ) + ")\n" );

		for( std::size_t i = 0; i < m_books.size(); ++i )
		{
			resp.append_body( std::to_string( i + 1 ) + ". " );
			const auto & b = m_books[ i ];
			resp.append_body( b.m_title + "[" + b.m_author + "]\n" );
		}

		return resp.done();
	}

	auto on_book_get(
		const restinio::request_handle_t& req, rr::route_params_t params )
	{
		const auto booknum = restinio::cast_to< std::uint32_t >( params[ "booknum" ] );

		auto resp = init_resp( req->create_response() );

		if( 0 != booknum && booknum <= m_books.size() )
		{
			const auto & b = m_books[ booknum - 1 ];
			resp.set_body(
				"Book #" + std::to_string( booknum ) + " is: " +
					b.m_title + " [" + b.m_author + "]\n" );
		}
		else
		{
			resp.set_body(
				"No book with #" + std::to_string( booknum ) + "\n" );
		}

		return resp.done();
	}

	auto on_author_get(
		const restinio::request_handle_t& req, rr::route_params_t params )
	{
		auto resp = init_resp( req->create_response() );
		try
		{
			auto author = restinio::utils::unescape_percent_encoding( params[ "author" ] );

			resp.set_body( "Books of " + author + ":\n" );

			for( std::size_t i = 0; i < m_books.size(); ++i )
			{
				const auto & b = m_books[ i ];
				if( author == b.m_author )
				{
					resp.append_body( std::to_string( i + 1 ) + ". " );
					resp.append_body( b.m_title + "[" + b.m_author + "]\n" );
				}
			}
		}
		catch( const std::exception & )
		{
			mark_as_bad_request( resp );
		}

		return resp.done();
	}

	auto on_new_book(
		const restinio::request_handle_t& req, rr::route_params_t )
	{
		auto resp = init_resp( req->create_response() );

		try
		{
			m_books.emplace_back(
				json_dto::from_json< book_t >( req->body() ) );
		}
		catch( const std::exception & /*ex*/ )
		{
			mark_as_bad_request( resp );
		}

		return resp.done();
	}

	auto on_book_update(
		const restinio::request_handle_t& req, rr::route_params_t params )
	{
		const auto booknum = restinio::cast_to< std::uint32_t >( params[ "booknum" ] );

		auto resp = init_resp( req->create_response() );

		try
		{
			auto b = json_dto::from_json< book_t >( req->body() );

			if( 0 != booknum && booknum <= m_books.size() )
			{
				m_books[ booknum - 1 ] = b;
			}
			else
			{
				mark_as_bad_request( resp );
				resp.set_body( "No book with #" + std::to_string( booknum ) + "\n" );
			}
		}
		catch( const std::exception & /*ex*/ )
		{
			mark_as_bad_request( resp );
		}

		return resp.done();
	}

	auto on_book_delete(
		const restinio::request_handle_t& req, rr::route_params_t params )
	{
		const auto booknum = restinio::cast_to< std::uint32_t >( params[ "booknum" ] );

		auto resp = init_resp( req->create_response() );

		if( 0 != booknum && booknum <= m_books.size() )
		{
			const auto & b = m_books[ booknum - 1 ];
			resp.set_body(
				"Delete book #" + std::to_string( booknum ) + ": " +
					b.m_title + "[" + b.m_author + "]\n" );

			m_books.erase( m_books.begin() + ( booknum - 1 ) );
		}
		else
		{
			resp.set_body(
				"No book with #" + std::to_string( booknum ) + "\n" );
		}

		return resp.done();
	}

private :
	weatherStation_t & m_books;

	template < typename RESP >
	static RESP
	init_resp( RESP resp )
	{
		resp
			.append_header( "Server", "RESTinio sample server /v.0.6" )
			.append_header_date_field()
			.append_header( "Content-Type", "text/plain; charset=utf-8" );

		return resp;
	}

	template < typename RESP >
	static void
	mark_as_bad_request( RESP & resp )
	{
		resp.header().status_line( restinio::status_bad_request() );
	}
};

auto server_handler( weatherStation_t & weather_data )
{
	auto router = std::make_unique< router_t >();
	auto handler = std::make_shared< books_handler_t >( std::ref(weather_data) );

	auto by = [&]( auto method ) {
		using namespace std::placeholders;
		return std::bind( method, handler, _1, _2 );
	};

	auto method_not_allowed = []( const auto & req, auto ) {
			return req->create_response( restinio::status_method_not_allowed() )
					.connection_close()
					.done();
		};

	// Handlers for '/' path.
	router->http_get( "/", by( &books_handler_t::on_books_list ) );
	router->http_post( "/", by( &books_handler_t::on_new_book ) );

	// Disable all other methods for '/'.
	router->add_handler(
			restinio::router::none_of_methods(
					restinio::http_method_get(), restinio::http_method_post() ),
			"/", method_not_allowed );

	// Handler for '/author/:author' path.
	router->http_get( "/author/:author", by( &books_handler_t::on_author_get ) );

	// Disable all other methods for '/author/:author'.
	router->add_handler(
			restinio::router::none_of_methods( restinio::http_method_get() ),
			"/author/:author", method_not_allowed );

	// Handlers for '/:booknum' path.
	router->http_get(
			R"(/:booknum(\d+))",
			by( &books_handler_t::on_book_get ) );
	router->http_put(
			R"(/:booknum(\d+))",
			by( &books_handler_t::on_book_update ) );
	router->http_delete(
			R"(/:booknum(\d+))",
			by( &books_handler_t::on_book_delete ) );

	// Disable all other methods for '/:booknum'.
	router->add_handler(
			restinio::router::none_of_methods(
					restinio::http_method_get(),
					restinio::http_method_post(),
					restinio::http_method_delete() ),
			R"(/:booknum(\d+))", method_not_allowed );

	return router;
}

int main()
{
	using namespace std::chrono;

	try
	{
		using traits_t =
			restinio::traits_t<
				restinio::asio_timer_manager_t,
				restinio::single_threaded_ostream_logger_t,
				router_t >;

		weatherStation_t weather_data{
			{ "Agatha Christie", "Murder on the Orient Express" },
			{ "Agatha Christie", "Sleeping Murder" },
			{ "B. Stroustrup", "The C++ Programming Language" }
		};

		restinio::run(
			restinio::on_this_thread< traits_t >()
				.address( "localhost" )
				.request_handler( server_handler( weather_data ) )
				.read_next_http_message_timelimit( 10s )
				.write_http_response_timelimit( 1s )
				.handle_request_timeout( 1s ) );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

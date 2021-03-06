/*
 * A test for redirection of mutable as immutable messages.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

class sobj_message_tester final : public so_5::agent_t
{
	struct hello final : public so_5::message_t
	{
		std::string m_content;
		hello( std::string content ) : m_content( std::move(content) ) {}
	};

public :
	sobj_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &sobj_message_tester::on_hello_mut );
		so_subscribe_self().event( &sobj_message_tester::on_hello_imm );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg< hello > >( *this, "hello" );
	}

private :
	const hello * m_received_ptr{ nullptr };

	void on_hello_mut( mhood_t< so_5::mutable_msg< hello > > cmd )
	{
		ensure(
				so_5::message_mutability_t::mutable_message ==
						message_mutability( *cmd ),
				"mutable message is expected" );

		std::cout << "sobj: " << cmd->m_content << std::endl;

		m_received_ptr = cmd.get();
		cmd->m_content = "bye";
		send( *this, to_immutable( std::move(cmd) ) );
	}

	void on_hello_imm( mhood_t< hello > cmd )
	{
		ensure(
				so_5::message_mutability_t::immutable_message ==
						message_mutability( *cmd ),
				"immutable message is expected" );

		std::cout << "sobj: " << cmd->m_content << std::endl;

		ensure( m_received_ptr == cmd.get(), "expect the same message" );
		ensure( "bye" == cmd->m_content, "expect 'bye' message" );

		so_deregister_agent_coop_normally();
	}
};

class user_message_tester final : public so_5::agent_t
{
	struct hello final
	{
		std::string m_content;
		hello( std::string content ) : m_content( std::move(content) ) {}
	};

public :
	user_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &user_message_tester::on_hello_mut );
		so_subscribe_self().event( &user_message_tester::on_hello_imm );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg< hello > >( *this, "hello" );
	}

private :
	const hello * m_received_ptr{ nullptr };

	void on_hello_mut( mhood_t< so_5::mutable_msg< hello > > cmd )
	{
		std::cout << "user: " << cmd->m_content << std::endl;

		m_received_ptr = cmd.get();
		cmd->m_content = "bye";
		send( *this, to_immutable( std::move(cmd) ) );
	}

	void on_hello_imm( mhood_t< hello > cmd )
	{
		std::cout << "user: " << cmd->m_content << std::endl;

		ensure( m_received_ptr == cmd.get(), "expect the same message" );
		ensure( "bye" == cmd->m_content, "expect 'bye' message" );

		so_deregister_agent_coop_normally();
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						env.register_agent_as_coop(
								env.make_agent<sobj_message_tester>());

						env.register_agent_as_coop(
								env.make_agent<user_message_tester>());
					},
					[](so_5::environment_params_t & params) {
						(void)params;
#if 0
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
#endif
					} );
			},
			5,
			"simple agent");
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


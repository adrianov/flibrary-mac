#include "AnnotationControllerJoke.h"

namespace HomeCompa::Flibrary
{

namespace
{

class JokeRequesterClientImpl final : virtual public IJokeRequester::IClient
{
public:
	explicit JokeRequesterClientImpl(IClient& impl)
		: m_impl(impl)
	{
	}

private: // IJokeRequester::IClient
	void OnError(const QString& api, const QString& error) override
	{
		m_impl.OnError(api, error);
	}

	void OnTextReceived(const QString& value) override
	{
		m_impl.OnTextReceived(value);
	}

	void OnImageReceived(const QByteArray& value) override
	{
		m_impl.OnImageReceived(value);
	}

private:
	IClient& m_impl;
};

} // namespace

std::shared_ptr<IJokeRequester::IClient> CreateJokeRequesterClient(IJokeRequester::IClient& impl)
{
	return std::make_shared<JokeRequesterClientImpl>(impl);
}

}

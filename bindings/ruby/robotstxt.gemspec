# frozen_string_literal: true

Gem::Specification.new do |spec|
  spec.name          = 'robotstxt'
  spec.version       = '1.1.0'
  spec.authors       = ['Google LLC']
  spec.email         = ['']

  spec.summary       = "Ruby bindings for Google's robots.txt parser library"
  spec.description   = 'A Ruby wrapper around Google\'s robots.txt parser library, ' \
                       'providing RFC 9309 compliant robots.txt parsing with support for ' \
                       'crawl-delay, request-rate, and content-signal directives.'
  spec.homepage      = 'https://github.com/nzrsky/robotstxt'
  spec.license       = 'Apache-2.0'

  spec.required_ruby_version = '>= 2.7.0'

  spec.files = Dir['lib/**/*.rb'] + ['README.md', 'LICENSE']
  spec.require_paths = ['lib']

  spec.add_dependency 'ffi', '~> 1.15'

  spec.add_development_dependency 'minitest', '~> 5.0'
  spec.add_development_dependency 'rake', '~> 13.0'
  spec.add_development_dependency 'rubocop', '~> 1.0'
end
